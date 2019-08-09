
# HDLCompiler

Compiler for a self-invented hardware description language.

The front end uses a hand-written recursive descent parser (`LL(1)`) to parse the source language, does semantic analysis and emits intermediate code. This intermediate code is then linked and can be translated to different target formats.

## Compiler usage

The compiler consists of a single executable, `hdlc`. It handles parsing, intermediate code generation, linking and target code generation.

```
USAGE:
    hdlc [FLAGS] [OPTIONS] <FILE>...

FLAGS:
    -d               Treat all input files as intermediate code and dump them
    -c               Compile each input file separately to intermediate code
    -h, --help       Prints help information
    -l               Compile the input files to intermediate code and link them, but don't run the backend
    -V, --version    Prints version information
    -v               Sets the level of verbosity

OPTIONS:
    -b <BACKEND>        Use a specific backend [possible values: LogicSimulator]
    -B=<ARG>...         Pass argument ARG to the backend
    -x <TYPE>...        Treat subsequent input files as having type TYPE [possible values: auto, hdl]
    -o <FILE>           Write output to FILE

ARGS:
    <FILE>...    The input files
```

Each input file is first compiled to intermediate code internally. Source files will be parsed, analyzed and compiled to intermediate code. Intermediate code files will just be read into memory. If the `-c` flag was passed, the compilation process ends here and the intermediate code files are written to disk.

All the intermediate code files will then be linked together to a single file. If the `-l` flag was passed, this file will be written to disk and no further processing will be done.

Then, the backend specified with the `-b` flag (`LogicSimulator` by default) is invoked, which compiles the single intermediate file to the target format.

The complete compilation process looks like this:
```
 +-------------+    /-----------\    +-------------------+
 | source file | -> | front end | -> | intermediate code | -+
 +-------------+    \-----------/    +-------------------+  |   /--------\    +-------------------+    /----------\    +-------------+
                                                            +-> | linker | -> | intermediate code | -> | back end | -> | target file |
                                     +-------------------+  |   \--------/    +-------------------+    \----------/    +-------------+
                                     | intermediate code | -+
                                     +-------------------+  |

                                                           ...
```

### Backends

The compiler supports different compilation backends:

#### LogicSimulator

This backend generates circuits for https://github.com/LogicSimulator, my editor and simulator for digital circuits.

It produces a `.json` file, which can be loaded in the simulator by pressing the folder icon in the top left corner.

The leftmost vertical signals are the inputs followed by the outputs of the circuit. For example, if a circuit is generated from the following description:
```
block main {
    in a;
    in[2] b;
    out q;
    out[2] r;
    
    q = a;
    r = b;
}
```
it will have 3 input signals and 3 output signals. The leftmost signal corresponds to input `a`. The next two signals correspond to input `b` with the least-significant bit first. The fourth signal from the left is output `q`, followed by two signals for output `r`.

The backend generates toggle buttons and LEDs for the inputs/outputs of the main block. You can pass it the flag `--no-io-components` (e.g. `hdlc -B --no-io-components circuit.hdl`) if you don't want that.

### Invocation examples

Compile a single source file, `a.hdl`, to a LogicSimulator circuit, `circuit.json`:
```
hdlc a.hdl
```

Compile two source files, `a.hdl` and `b.hdl`, to a LogicSimulator circuit, `a_and_b.json`:
```
hdlc a.hdl b.hdl -o a_and_b.json
```

Compile two source files, `a.hdl` and `b.hdl`, to separate intermediate code files, `a.hdli` and `b.hdli`:
```
hdlc -c a.hdl b.hdl
```

Link two intermediate code files, `a.hdli` and `b.hdli`, and write the resulting intermediate code file to `a_and_b.hdli`:
```
hdlc -l a.hdl b.hdl -o a_and_b.hdli
```

Compile one intermediate code file, `a_and_b.hdli`, to a LogicSimulator circuit, `a_and_b.json`:
```
hdlc a_and_b.hdli -o a_and_b.json
```

## The language

The hardware description languages syntax is inspired by C. Blocks are surrounded by braces, statements are terminated with semicolons.

A circuit is composed of named "blocks". A single `.hdl` file can contain multiple block definitions. Each block has inputs and outputs, which have names and can be of any width (other than zero, obviously). It can also make use of other blocks, which enables a very modular structure. The inputs, outputs and used blocks have to be declared at the start of a block:
```
block some_block {
    in a, b;
    in[25] wide_input, anotherWideInput;
    out[2] Q, output2;
    block some_other_block b1, b2;
}
```

Each circuit has to have exactly one block named `"main"`.

Following the declarations, the behaviour of the block is specified. You can write simple assignment statements, with an output on the left and an expression on the right. Available operators are the binary operators `&` (AND), `|` (OR) and `^` (XOR) and the unary operator `~` (NOT). If the operands are multiple bits wide, the operators apply to each bit individually. Sub-expressions can be parenthesized. Operands can either be number literals or identifiers. You can extract a single or multiple bits from a signal with "array accesses". Use `id[index]` to access a single bit and `id[index:index]` to access a range of bits (the left index must be the higher one). To access inputs and outputs of a sub-block, use the name of the block followed by a dot and the input/output name: `block_name.input_or_output_name`.

As a complete example take this 4-bit adder, which combines four single-bit adders:
```
block adder {
    in a, b, c_in;
    out s, c_out;

    s = a ^ b ^ c_in;
    c_out = a & b | c_in & (a ^ b);
}

block main {
    in[4] a, b;
    in c_in;
    out[4] s;
    out c_out;
    block adder adder0, adder1, adder2, adder3;

    adder0.a = a[0];
    adder0.b = b[0];
    adder0.c_in = c_in;
    s[0] = adder0.s;

    adder1.a = a[1];
    adder1.b = b[1];
    adder1.c_in = adder0.c_out;
    s[1] = adder1.s;

    adder2.a = a[2];
    adder2.b = b[2];
    adder2.c_in = adder1.c_out;
    s[2] = adder2.s;

    adder3.a = a[3];
    adder3.b = b[3];
    adder3.c_in = adder2.c_out;
    s[3] = adder3.s;

    c_out = adder3.c_out;
}
```

## Building

The project is written in Rust and therefore needs `rustc` and `cargo`.

To build it:
```sh
cargo build
# or
cargo build --release
```
The executable is then located at `target/debug/hdlc` or `target/release/hdlc`.

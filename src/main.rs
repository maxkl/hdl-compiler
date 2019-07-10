
use std::env;

use hdl_compiler::run;

fn main() {
    let args = env::args().collect();

    if let Err(err) = run(args) {
        eprintln!("{}", err);
    }
}


use std::error::Error;
use std::env;

use hdl_compiler::run;

fn main() {
    let args = env::args().collect();

    if let Err(err) = run(args) {
        eprintln!("error: {}", err);

        let mut err: &dyn Error = &err;
        while let Some(source) = err.source() {
            eprintln!("caused by: {}", source);
            err = source;
        }
    }
}

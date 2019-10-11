
mod shared;
mod frontend;
mod backend;

use std::path::Path;
use std::ffi::OsStr;

use clap::{App, Arg};
use derive_more::Display;

use crate::shared::intermediate::Intermediate;
use crate::shared::error;

const FRONTENDS: [(&str, fn(&Path) -> frontend::Result); 1] = [
    ("hdl", frontend::compile),
];

const BACKENDS: [(&str, fn(Option<&str>, &Intermediate, &[&str]) -> backend::Result); 1] = [
    ("LogicSimulator", backend::logic_simulator::LogicSimulator::run),
];

const INPUT_FILE_TYPES: [(&str, &str); 2] = [
    ("hdl", "hdl"),
    ("hdli", "intermediate"),
];

#[derive(Debug, Display)]
pub enum ErrorKind {
    #[display(fmt = "failed to compile {}", _0)]
    Compile(String),
    #[display(fmt = "error in backend")]
    Backend,
    #[display(fmt = "{}: file format not recognized", _0)]
    FileFormatNotRecognized(String),
    #[display(fmt = "feature not implemented yet")]
    NotImplemented,
}

pub type Error = error::Error<ErrorKind>;

pub fn run(args: Vec<String>) -> Result<(), Error> {
    let frontend_names = FRONTENDS.iter()
            .map(|frontend| frontend.0)
        .collect::<Vec<_>>();

    let backend_names = BACKENDS.iter()
        .map(|backend| backend.0)
        .collect::<Vec<_>>();
    let default_backend = backend_names[0];

    let matches = App::new("HDLCompiler")
        .version("0.1")
        .author("Max Klein <max@maxkl.de>")
        .arg(Arg::with_name("input_file_type")
            .short("t")
            .value_name("TYPE")
            .help("Treat the input file as having type TYPE. If not specified, the type is guessed based on the file extension")
            .possible_values(&frontend_names))
        .arg(Arg::with_name("dump_intermediate")
            .short("d")
            .help("Treat the input file as intermediate code and pretty-print it"))
        .arg(Arg::with_name("frontend_only")
            .short("i")
            .help("Compile the input file to intermediate code"))
        .arg(Arg::with_name("backend")
            .short("b")
            .value_name("BACKEND")
            .help("Use a specific backend")
            .possible_values(&backend_names))
        .arg(Arg::with_name("output_file")
            .short("o")
            .value_name("FILE")
            .help("Write output to FILE"))
        .arg(Arg::with_name("backend_args")
            .short("B")
            .value_name("ARG")
            .help("Pass comma-separated list of arguments to the backend")
            .multiple(true)
            .require_delimiter(true)
            .require_equals(true))
        .arg(Arg::with_name("verbosity")
            .short("v")
            .multiple(true)
            .help("Increase logging verbosity"))
        .arg(Arg::with_name("input_file")
            .value_name("FILE")
            .help("The input file")
            .required(true))
        .get_matches_from(args);

    let verbosity = matches.occurrences_of("verbosity");

    let dump_intermediate = matches.is_present("dump_intermediate");

    let frontend_only = matches.is_present("frontend_only");

    let backend_name = matches.value_of("backend").unwrap_or(default_backend);

    let output_file = matches.value_of("output_file");

    let input_file_name = matches.value_of("input_file").unwrap();
    let input_file_type = matches.value_of("input_file_type");

    let backend_args = matches.values_of("backend_args")
        .map(|backend_args| backend_args.collect::<Vec<_>>())
        .unwrap_or(Vec::new());

    if dump_intermediate {
        // TODO: read and dump intermediate files
        return Err(ErrorKind::NotImplemented.into());
    }

    let input_path = Path::new(input_file_name);

    let frontend_name = if let Some(value) = input_file_type {
        value
    } else {
        input_path.extension()
            .and_then(OsStr::to_str)
            .and_then(|ext| INPUT_FILE_TYPES.iter()
                .find_map(|file_type| if file_type.0 == ext {
                    Some(file_type.1)
                } else {
                    None
                }))
            .ok_or_else(|| ErrorKind::FileFormatNotRecognized(input_file_name.to_string()))?
    };

    let frontend_fn = FRONTENDS.iter()
        .find(|frontend| frontend.0 == frontend_name)
        .unwrap().1;

    let intermediate = frontend_fn(input_path)
        .map_err(|err| Error::with_source(ErrorKind::Compile(input_path.to_str().unwrap().to_string()), err))?;

    if frontend_only {
        // TODO: output intermediate file
        return Err(ErrorKind::NotImplemented.into());
    } else {
        let backend_fn = BACKENDS.iter()
            .find(|backend| backend.0 == backend_name)
            .unwrap().1;

        backend_fn(output_file, &intermediate, backend_args.as_slice())
            .map_err(|err| Error::with_source(ErrorKind::Backend, err))?;
    }

    Ok(())
}

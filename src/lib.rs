
mod shared;
mod frontend;
mod linker;
mod backend;

use std::io;
use std::fs::File;
use std::path::Path;
use std::ffi::OsStr;

use clap::{App, Arg};
use derive_more::Display;

use crate::shared::intermediate::Intermediate;
use crate::linker::Linker;
use crate::shared::error;

const FRONTENDS: [(&str, fn(frontend::Input) -> frontend::Result); 1] = [
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
    #[display(fmt = "failed to link intermediate files")]
    Linker,
    #[display(fmt = "error in backend")]
    Backend,
    #[display(fmt = "failed to open file {}", _0)]
    FileOpen(String),
    #[display(fmt = "-o specified for multiple output files")]
    MultipleOutputsFileSpecified,
    #[display(fmt = "reading from standard input requires explicit file type")]
    StdinFileType,
    #[display(fmt = "{}: file format not recognized", _0)]
    FileFormatNotRecognized(String),
    #[display(fmt = "feature not implemented yet")]
    NotImplemented,
}

pub type Error = error::Error<ErrorKind>;

pub fn run(args: Vec<String>) -> Result<(), Error> {
    let frontend_names = {
        let mut names = vec!["auto"];

        names.extend(FRONTENDS.iter()
            .map(|frontend| frontend.0));

        names
    };
    let default_frontend = frontend_names[0];

    let backend_names = BACKENDS.iter()
        .map(|backend| backend.0)
        .collect::<Vec<_>>();
    let default_backend = backend_names[0];

    let matches = App::new("HDLCompiler")
        .version("0.1")
        .author("Max Klein <max@maxkl.de>")
        .arg(Arg::with_name("input_file_types")
            .short("x")
            .value_name("TYPE")
            .help("Treat subsequent input files as having type TYPE")
            .multiple(true)
            .number_of_values(1)
            .possible_values(&frontend_names))
        .arg(Arg::with_name("dump_intermediate")
            .short("d")
            .help("Treat all input files as intermediate code and dump them"))
        .arg(Arg::with_name("frontend_only")
            .short("c")
            .help("Compile each input file separately to intermediate code"))
        .arg(Arg::with_name("link_only")
            .short("l")
            .help("Compile the input files to intermediate code and link them, but don't run the backend"))
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
            .help("Pass argument ARG to the backend")
            .multiple(true)
            .require_delimiter(true)
            .require_equals(true))
        .arg(Arg::with_name("verbosity")
            .short("v")
            .multiple(true)
            .help("Sets the level of verbosity"))
        .arg(Arg::with_name("input_files")
            .value_name("FILE")
            .help("The input files")
            .multiple(true)
            .required(true))
        .get_matches_from(args);

    let verbosity = matches.occurrences_of("verbosity");

    let dump_intermediate = matches.is_present("dump_intermediate");

    let frontend_only = matches.is_present("frontend_only");

    let link_only = matches.is_present("link_only");

    let backend_name = matches.value_of("backend").unwrap_or(default_backend);

    let output_file = matches.value_of("output_file");

    let input_files = if let Some(input_files) = matches.values_of("input_files") {
        let input_file_indices = matches.indices_of("input_files").unwrap();

        let input_files_iter = input_files.zip(input_file_indices);

        let input_types = if let Some(input_types) = matches.values_of("input_file_types") {
            let input_type_indices = matches.indices_of("input_file_types").unwrap();

            input_types.zip(input_type_indices)
                .collect::<Vec<_>>()
        } else {
            Vec::new()
        };

        let input_files_with_type = input_files_iter
            .map(|input_file| {
                let next_index = input_types
                    .binary_search_by_key(&input_file.1, |input_type| input_type.1)
                    .unwrap_err();

                let input_type = if next_index > 0 {
                    input_types[next_index - 1].0
                } else {
                    default_frontend
                };

                (input_file.0, input_type)
            })
            .collect::<Vec<_>>();

        input_files_with_type
    } else {
        Vec::new()
    };

    let backend_args = matches.values_of("backend_args")
        .map(|backend_args| backend_args.collect::<Vec<_>>())
        .unwrap_or(Vec::new());

    if dump_intermediate {
        // TODO: read and dump intermediate files
        return Err(ErrorKind::NotImplemented.into());
    }

    if input_files.len() > 0 && frontend_only && output_file.is_some() {
        return Err(ErrorKind::MultipleOutputsFileSpecified.into());
    }

    let mut intermediate_files = Vec::new();

    for input_file in input_files {
        let frontend_name = if input_file.1 == "auto" {
            if input_file.0 == "-" {
                return Err(ErrorKind::StdinFileType.into());
            }

            Path::new(input_file.0)
                .extension()
                .and_then(OsStr::to_str)
                .and_then(|ext| INPUT_FILE_TYPES.iter()
                    .find_map(|file_type| if file_type.0 == ext {
                        Some(file_type.1)
                    } else {
                        None
                    }))
                .ok_or_else(|| ErrorKind::FileFormatNotRecognized(input_file.0.to_string()))?
        } else {
            input_file.1
        };

        let frontend_fn = FRONTENDS.iter()
            .find(|frontend| frontend.0 == frontend_name)
            .unwrap().1;

        let stdin;
        let f;

        let input = if input_file.0 == "-" {
            stdin = io::stdin();
            frontend::Input::from_stdin(&stdin)
        } else {
            f = File::open(input_file.0)
                .map_err(|err| Error::with_source(ErrorKind::FileOpen(input_file.0.to_string()), err))?;
            frontend::Input::from_file(&f)
        };

        let intermediate = frontend_fn(input)
            .map_err(|err| {
                let file_name = if input_file.0 == "-" {
                    "standard input".to_string()
                } else {
                    input_file.0.to_string()
                };
                Error::with_source(ErrorKind::Compile(file_name), err)
            })?;

        if frontend_only {
            // TODO: output intermediate file
            return Err(ErrorKind::NotImplemented.into());
        } else {
            intermediate_files.push(intermediate);
        }
    }

    if !frontend_only {
        let linked_intermediate = Linker::link(&intermediate_files)
            .map_err(|err| Error::with_source(ErrorKind::Linker, err))?;

        if link_only {
            // TODO: output intermediate file
            return Err(ErrorKind::NotImplemented.into());
        } else {
            let backend_fn = BACKENDS.iter()
                .find(|backend| backend.0 == backend_name)
                .unwrap().1;

            backend_fn(output_file, &linked_intermediate, backend_args.as_slice())
                .map_err(|err| Error::with_source(ErrorKind::Backend, err))?;
        }
    }

    Ok(())
}

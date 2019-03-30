
#include <iostream>
#include <filesystem>
#include <getopt.h>

#include <shared/Intermediate.h>
#include <shared/IntermediateFile.h>
#include <frontend/Frontend.h>
#include <linker/Linker.h>
#include <backend/LogicSimulator/LogicSimulator.h>
#include <shared/helper/string_format.h>
#include <shared/errors.h>

using namespace HDLCompiler;

static const char short_options[] = "-:x:dclb:o:B:v::Vh";
static const struct option long_options[] = {
	{ "version", no_argument, nullptr, 'V' },
	{ "help", no_argument, nullptr, 'h' },
	{ nullptr, 0, nullptr, 0 }
};

typedef void (*BackendRunFunction)(std::string outputFilename, const Intermediate::File &intermediateFile, const std::vector<std::string> &argv);

static const std::vector<std::pair<std::string, BackendRunFunction>> backends {
    {"LogicSimulator", Backend::LogicSimulator::run},
    {"csim", nullptr}
};

typedef Intermediate::File (*InputFileProcessingFunction)(const std::string &filename);

static const std::vector<std::pair<std::string, InputFileProcessingFunction>> inputFileTypes {
    {"hdl", Frontend::compile},
    {"intermediate", Intermediate::File::read}
};

static const std::vector<std::pair<std::string, std::string>> inputFileExtensions {
    {"hdl", "hdl"},
    {"*", "intermediate"}
};

struct InputFile {
	std::string name;
	int type;

	InputFile(std::string name, int type)
	    : name(std::move(name)), type(type) {
	    //
	}
};

int clamp(int v, int min, int max) {
	return v < min ? min : v > max ? max : v;
}

std::string basename(std::string path) {
    std::size_t slashIndex = path.find_last_of("/\\");
    if (slashIndex != std::string::npos) {
        path.erase(0, slashIndex + 1);
    }

    return path;
}

/**
 * Get file extension from a path:
 * "file.ext" -> "ext"
 * "file" -> ""
 * "path.ext/file" -> ""
 * ".file" -> ""
 *
 * @param path
 * @return
 */
std::string getExtension(const std::string &path) {
    auto slashIndex = path.find_last_of("/\\");
    auto dotIndex = path.find_last_of('.');
	if (dotIndex == std::string::npos || dotIndex == 0 || (slashIndex != std::string::npos && dotIndex < slashIndex)) {
		return "";
	}
	return path.substr(dotIndex + 1);
}

std::string replaceExtension(std::string path, const std::string &newExtension) {
    auto slashIndex = path.find_last_of("/\\");
    auto dotIndex = path.find_last_of('.');
    if (dotIndex == std::string::npos || dotIndex == 0 || (slashIndex != std::string::npos && dotIndex < slashIndex)) {
        // no extension to remove
    } else {
        path.erase(dotIndex);
    }
    return path + '.' + newExtension;
}

void printVersion(const std::string &programName) {
    std::cout << programName << " x.x.x\n";
}

void printHelp(const std::string &programName) {
	std::cout << "Usage:\n";
	std::cout << "  " << programName << " [option]... <file>...\n";
	std::cout << "\n";
	std::cout << "Options:\n";
	std::cout << "  -x <type>     Treat subsequent input files as having type <type>.\n";
	std::cout << "                Available types are: 'auto' (default)";
	for (const auto &inputFileType : inputFileTypes) {
	    std::cout << ", '" << inputFileType.first << "'";
	}
	std::cout << "\n";
	std::cout << "                If 'auto' is specified, the type is guessed based on the file extension.\n";
	std::cout << "  -d            Treat all input files as intermediate code and dump them.\n";
	std::cout << "  -c            Compile each input file separately to intermediate code.\n";
	std::cout << "  -l            Compile the input files to intermediate code and link them, but don't run the backend.\n";
	std::cout << "  -b <backend>  Use a specific backend.\n";
	std::cout << "                Available backends are: ";
	for (std::size_t i = 0; i < backends.size(); i++) {
	    std::cout << "'" << backends[i].first << "'";
	    if (i == 0) {
	        std::cout << " (default)";
	    }
	    if (i < backends.size() - 1) {
	        std::cout << ", ";
	    }
	}
	std::cout << "\n";
	std::cout << "  -o <file>     Write output to <file>.\n";
	std::cout << "  -B <arg>      Pass argument <arg> to the backend.\n";
	std::cout << "  -v[level]     Print more log messages.\n";
	std::cout << "  -V, --version Display version information and exit.\n";
	std::cout << "  -h, --help    Display this help and exit.\n";
}

int detectInputFileType(const std::string &path, int defaultType) {
    if (defaultType != -1) {
        return defaultType;
    }

    std::string extension = getExtension(path);

    const std::string *inputFileTypeStr = nullptr;

    for (const auto &inputFileExtension : inputFileExtensions) {
        if (inputFileExtension.first == extension || inputFileExtension.first == "*") {
            inputFileTypeStr = &inputFileExtension.second;
            break;
        }
    }

    if (inputFileTypeStr == nullptr) {
        throw CompilerError("Unable to auto-detect file type of '" + path + "'");
    }

    for (std::size_t i = 0; i < inputFileTypes.size(); i++) {
        if (inputFileTypes[i].first == *inputFileTypeStr) {
            return i;
        }
    }

    throw CompilerError("Unrecognized input file type '" + *inputFileTypeStr + "' (auto-detected)");
}

int main(int argc, char **argv) {
	std::string programName = basename(argv[0]);

	int inputFileTypeOption = -1;
	bool dumpIntermediateOption = false;
	bool frontendOnlyOption = false;
	bool linkOnlyOption = false;
	int backendOption = 0;
	std::string outputFileNameOption;
	std::vector<std::string> backendArgv;
	int verboseOption = 0;
	bool versionOption = false;
	bool helpOption = false;

	std::vector<InputFile> inputFiles;

	opterr = 0;

	int c;
	while ((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
	    // Avoid string construction from nullptr
	    std::string optarg_str(optarg ? optarg : "");
		switch (c) {
			case 1:
                inputFiles.emplace_back(optarg_str, detectInputFileType(optarg_str, inputFileTypeOption));
                break;
			case 'x': {
			    if (optarg_str == "auto") {
			        inputFileTypeOption = -1;
			        break;
			    }

			    bool foundInputFileType = false;
                for (std::size_t i = 0; i < inputFileTypes.size(); i++) {
                    if (inputFileTypes[i].first == optarg_str) {
                        inputFileTypeOption = i;
                        foundInputFileType = true;
                        break;
                    }
                }
                if (!foundInputFileType) {
                    throw CompilerError("Unrecognized input file type '" + optarg_str + "'");
                }
				break;
			}
			case 'd':
				dumpIntermediateOption = true;
				break;
			case 'c':
				frontendOnlyOption = true;
				break;
			case 'l':
				linkOnlyOption = true;
				break;
			case 'b': {
                bool foundBackend = false;
                for (std::size_t i = 0; i < backends.size(); i++) {
                    if (backends[i].first == optarg_str) {
                        backendOption = i;
                        foundBackend = true;
                        break;
                    }
                }
                if (!foundBackend) {
                    throw CompilerError("Unrecognized backend name '" + optarg_str + "'");
                }
                break;
			}
			case 'o':
				outputFileNameOption = optarg_str;
				break;
			case 'B':
			    backendArgv.push_back(optarg_str);
				break;
			case 'v':
				if (optarg == nullptr) {
					verboseOption = 1;
				} else {
					int value;

					try {
					    value = std::stoi(optarg_str);
					} catch (std::invalid_argument &e) {
					    throw CompilerError("Invalid integer argument '" + optarg_str + "'");
					} catch (std::out_of_range &e) {
					    throw CompilerError("Integer argument " + optarg_str + " is out of range");
					}

					verboseOption = clamp(value, 0, 2);
				}
				break;
			case 'V':
				versionOption = true;
				break;
			case 'h':
				helpOption = true;
				break;
			case ':': {
                std::string message = "Argument to '-" + std::string(1, optopt) + "'";
                for (std::size_t i = 0; long_options[i].name; i++) {
                    if (long_options[i].val == optopt) {
                        message += "/'--" + std::string(long_options[i].name) + "'";
                    }
                }
                message += " is missing";
                throw CompilerError(message);
            }
			case '?':
				if (optopt == 0) {
                    throw CompilerError(string_format("Unrecognized option '%s'", argv[optind - 1]));
				} else if (isprint(optopt)) {
                    throw CompilerError(string_format("Unrecognized option '-%c'", optopt));
				} else {
                    throw CompilerError(string_format("Unrecognized option character '\\x%x'", optopt));
				}
			default:
				abort();
		}
	}

	if (helpOption) {
		printHelp(programName);
		return 0;
	}

	if (versionOption) {
		printVersion(programName);
		return 0;
	}

	for (int i = optind; i < argc; i++) {
		std::string path(argv[i]);
        inputFiles.emplace_back(path, detectInputFileType(path, inputFileTypeOption));
	}

	if (verboseOption > 0) {
	    std::cout << "enabled verbose logging, level " << verboseOption << "\n";
	}

	if (inputFiles.empty()) {
		throw CompilerError("No input files");
	}

	if (dumpIntermediateOption) {
		for (const auto &inputFile : inputFiles) {
		    Intermediate::File intermediateFile = Intermediate::File::read(inputFile.name);

		    std::cout << inputFile.name << ":\n";
		    intermediateFile.print(std::cout);
		}

		return 0;
	}

	if (inputFiles.size() > 1 && frontendOnlyOption && !outputFileNameOption.empty()) {
		throw CompilerError("'-o' specified for multiple output files");
	}

	std::vector<Intermediate::File> intermediateFiles;

	for (const auto &inputFile : inputFiles) {
	    intermediateFiles.push_back(inputFileTypes[inputFile.type].second(inputFile.name));

        const Intermediate::File &intermediateFile = intermediateFiles.back();

		if (frontendOnlyOption) {
			std::string outputFileName;

			if (inputFiles.size() == 1 && !outputFileNameOption.empty()) {
				outputFileName = outputFileNameOption;
			} else {
				outputFileName = replaceExtension(inputFile.name, "hdli");
			}

			intermediateFile.write(outputFileName);
		}
	}

	if (!frontendOnlyOption) {
		Intermediate::File intermediateFile = Linker::link(intermediateFiles);

		if (linkOnlyOption) {
            std::string outputFileName = outputFileNameOption.empty() ? "linked.hdli" : outputFileNameOption;

			intermediateFile.write(outputFileName);

		} else {
			backends[backendOption].second(outputFileNameOption, intermediateFile, backendArgv);
		}
	}

	return 0;
}

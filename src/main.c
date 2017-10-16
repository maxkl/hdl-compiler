
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#include <shared/helper/mem.h>

static const char short_options[] = "-:x:clb:o:v::Vh";
static const struct option long_options[] = {
	{ "version", no_argument, NULL, 'V' },
	{ "help", no_argument, NULL, 'h' },
	{ NULL, 0, NULL, 0 }
};

static const struct {
	const char *name;
	int (* fn)();
} backends[] = {
	{ "LogicSimulator", NULL },
	{ "csim", NULL },
	{ NULL, NULL }
};

static const struct {
	const char *name;
	int (* fn)();
} input_file_types[] = {
	{ "hdl", NULL },
	{ "intermediate", NULL },
	{ NULL, NULL }
};

static const struct {
	const char *extension;
	const char *type;
} input_file_extensions[] = {
	{ "hdl", "hdl" },
	{ "*", "intermediate" },
	{ NULL, NULL }
};

struct input_file {
	const char *name;
	int type;
};

bool parse_long(const char *str, long *value_out) {
	char *end;
	long value = strtol(str, &end, 10);

	// Invalid format
	if (end == str || *end != '\0') {
		return false;
	}

	// Over- or underflow
	if ((value == LONG_MIN || value == LONG_MAX) && errno == ERANGE) {
		return false;
	}

	*value_out = value;

	return true;
}

int clamp(int v, int min, int max) {
	return v < min ? min : v > max ? max : v;
}

const char *get_extension(const char *filename) {
	const char *slash = strrchr(filename, '/');
	const char *dot = strrchr(filename, '.');
	if (dot == NULL || dot == filename || (slash != NULL && dot < slash)) {
		return "";
	}
	return dot + 1;
}

void print_version(const char *program_name) {
	printf("%s x.x.x\n", program_name);
}

void print_help(const char *program_name) {
	printf("Usage:\n");
	printf("  %s [option]... <file>...\n", program_name);
	printf("\n");
	printf("Options:\n");
	printf("  -x <type>     Treat subsequent input files as having type <type>.\n");
	printf("                Available types are: 'auto' (default)");
	for (int i = 0; input_file_types[i].name; i++) {
		printf(", '%s'", input_file_types[i].name);
	}
	printf("\n");
	printf("                If 'auto' is specified, the type is guessed based on the file extension.\n");
	printf("  -c            Compile each input file separately to intermediate code.\n");
	printf("  -l            Compile the input files to intermediate code and link them, but don't run the backend.\n");
	printf("  -b <backend>  Use a specific backend.\n");
	printf("                Available backends are: ");
	for (int i = 0; backends[i].name; i++) {
		printf("'%s'", backends[i].name);
		if (i == 0) {
			printf(" (default)");
		}
		if (backends[i + 1].name) {
			printf(", ");
		}
	}
	printf("\n");
	printf("  -o <file>     Write output to <file>.\n");
	printf("  -v[level]     Print more log messages.\n");
	printf("  -V, --version Display version information and exit.\n");
	printf("  -h, --help    Display this help and exit.\n");
}

int add_input_file(const char *name, int type, struct input_file **input_files, size_t *input_file_count) {
	if (type == -1) {
		const char *extension = get_extension(name);

		const char *input_file_type_str;

		bool found_extension = false;
		for (int i = 0; input_file_extensions[i].extension; i++) {
			if (strcmp(extension, input_file_extensions[i].extension) == 0 || strcmp(input_file_extensions[i].extension, "*") == 0) {
				input_file_type_str = input_file_extensions[i].type;
				found_extension = true;
				break;
			}
		}
		if (!found_extension) {
			fprintf(stderr, "error: unable to auto-detect file type of '%s'\n", name);
			return 1;
		}

		bool found_input_file_type = false;
		for (int i = 0; input_file_types[i].name; i++) {
			if (strcmp(input_file_type_str, input_file_types[i].name) == 0) {
				type = i;
				found_input_file_type = true;
				break;
			}
		}
		if (!found_input_file_type) {
			fprintf(stderr, "error: unrecognized input file type '%s' (auto-detected)\n", input_file_type_str);
			return 1;
		}
	}

	(*input_file_count)++;
	*input_files = xrealloc(*input_files, sizeof(struct input_file) * *input_file_count);
	struct input_file *input_file = &(*input_files)[*input_file_count - 1];
	input_file->name = name;
	input_file->type = type;

	return 0;
}

int main(int argc, char **argv) {
	int ret;

	char *argv0_copy = xstrdup(argv[0]);
	char *program_name = xstrdup(basename(argv0_copy));
	xfree(argv0_copy);

	int input_file_type_opt = -1;
	bool frontend_only_opt = false;
	bool link_only_opt = false;
	int backend_opt = 0;
	char *output_file_name_opt = NULL;
	int verbose_opt = 0;
	bool version_opt = false;
	bool help_opt = false;

	struct input_file *input_files = NULL;
	size_t input_file_count = 0;

	opterr = 0;

	int c;
	while ((c = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		switch (c) {
			case 1:
				ret = add_input_file(optarg, input_file_type_opt, &input_files, &input_file_count);
				if (ret) {
					return ret;
				}
				break;
			case 'x':
				if (strcmp(optarg, "auto") == 0) {
					input_file_type_opt = -1;
					break;
				}

				bool found_input_file_type = false;
				for (int i = 0; input_file_types[i].name; i++) {
					if (strcmp(optarg, input_file_types[i].name) == 0) {
						input_file_type_opt = i;
						found_input_file_type = true;
						break;
					}
				}
				if (!found_input_file_type) {
					fprintf(stderr, "error: unrecognized input file type '%s'\n", optarg);
					return 1;
				}
				break;
			case 'c':
				frontend_only_opt = true;
				break;
			case 'l':
				link_only_opt = true;
				break;
			case 'b': ;
				bool found_backend = false;
				for (int i = 0; backends[i].name; i++) {
					if (strcmp(optarg, backends[i].name) == 0) {
						backend_opt = i;
						found_backend = true;
						break;
					}
				}
				if (!found_backend) {
					fprintf(stderr, "error: unrecognized backend name '%s'\n", optarg);
					return 1;
				}
				break;
			case 'o':
				output_file_name_opt = optarg;
				break;
			case 'v':
				if (optarg == NULL) {
					verbose_opt = 1;
				} else {
					long value;

					if (!parse_long(optarg, &value)) {
						fprintf(stderr, "error: invalid integer argument '%s'\n", optarg);
						return 1;
					}

					if (value < INT_MIN || value > INT_MAX) {
						fprintf(stderr, "error: %li exceeds integer range\n", value);
						return 1;
					}

					verbose_opt = clamp(value, 0, 2);
				}
				break;
			case 'V':
				version_opt = true;
				break;
			case 'h':
				help_opt = true;
				break;
			case ':':
				fprintf(stderr, "error: argument to '-%c'", optopt);
				for (int i = 0; long_options[i].name; i++) {
					if (long_options[i].val == optopt) {
						fprintf(stderr, "/'--%s'", long_options[i].name);
					}
				}
				fprintf(stderr, " is missing\n");
				return 1;
			case '?':
				if (optopt == 0) {
					fprintf(stderr, "error: unrecognized option '%s'\n", argv[optind - 1]);
				} else if (isprint(optopt)) {
					fprintf(stderr, "error: unrecognized option '-%c'\n", optopt);
				} else {
					fprintf(stderr, "error: unrecognized option character '\\x%x'\n", optopt);
				}
				return 1;
			default:
				abort();
		}
	}

	if (help_opt) {
		print_help(program_name);
		return 0;
	}

	if (version_opt) {
		print_version(program_name);
		return 0;
	}

	for (int i = optind; i < argc; i++) {
		ret = add_input_file(argv[i], input_file_type_opt, &input_files, &input_file_count);
		if (ret) {
			return ret;
		}
	}

	if (input_file_count == 0) {
		fprintf(stderr, "error: no input files\n");
		return 1;
	}

	if (input_file_count > 1 && frontend_only_opt && output_file_name_opt != NULL) {
		fprintf(stderr, "error: -o specified for multiple output files\n");
		return 1;
	}

	if (verbose_opt > 0) {
		printf("enable verbose logging, level %i\n", verbose_opt);
	}

	for (size_t i = 0; i < input_file_count; i++) {
		struct input_file *input_file = &input_files[i];
		printf("compile %s file %s to intermediate\n", input_file_types[input_file->type].name, input_file->name);
		if (frontend_only_opt) {
			printf("write intermediate file to %s\n", output_file_name_opt);
		}
	}

	if (!frontend_only_opt) {
		printf("link all intermediate files together\n");
		if (link_only_opt) {
			printf("write linked intermediate file to %s\n", output_file_name_opt);
		} else {
			printf("run %s backend\n", backends[backend_opt].name);
			printf("write output to %s\n", output_file_name_opt);
		}
	}

	return 0;
}

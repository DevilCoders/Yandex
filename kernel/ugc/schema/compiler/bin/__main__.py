from kernel.ugc.schema.compiler.lib import SchemaGenerator, SchemaParser

import argparse
import json
import os.path
import sys


def main():
    parser = argparse.ArgumentParser(description="compiles schema file into C++ classes")
    parser.add_argument("--dump", action="store_true", help="dump compiled schema to stdout")
    parser.add_argument("--cpp", action="store_true", help="generate cpp files")
    parser.add_argument("--proto", action="store_true", help="generate proto files")
    parser.add_argument("schema_file", nargs="?", help="schema description file")
    args = parser.parse_args()

    if args.schema_file:
        schema_source = open(args.schema_file, "r").read()
        file_prefix = os.path.basename(args.schema_file)
    else:
        if not sys.stdin.isatty():
            schema_source = sys.stdin.read()
            file_prefix = "schema"
        else:
            parser.error("no schema file")

    schema = SchemaParser(schema_source).parse()

    if args.dump:
        print(json.dumps(schema, sort_keys=True, indent=4, separators=(",", ": ")))
        return

    generator = SchemaGenerator(schema)

    if file_prefix.endswith(".schema"):
        file_prefix = file_prefix[:-7]

    if args.cpp:
        file_name = "{prefix}.schema.h".format(prefix=file_prefix)
        with open(file_name, "w") as f:
            f.write(generator.generate_cpp())

    if args.proto:
        file_name = "{prefix}.proto".format(prefix=file_prefix)
        with open(file_name, "w") as f:
            f.write(generator.generate_proto())


if __name__ == "__main__":
    main()

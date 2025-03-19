#!/usr/bin/env python2
"""CLOUD-13497: simple linter for syntax validation of jinja templates"""

from collections import defaultdict
import errno
import os
import re
import shutil
import subprocess
import sys
import tempfile
import jinja2
import jinja2.ext


JINJA_EXTENSIONS = ("^.*\.(sls|conf|yaml|yml|service|j2|toml|jinja|properties|ini|txr|tpl|cfg)$", )
JINJA_BLACKLIST = ()


PYLINT_EXTENSIONS = ("^.*\.py$", )
PYLINT_BLACKLIST = ()
PYLINT_SUPPRESS_ERRORS = (
    "E0401",  # unable to import %s
    "E1101",  # no-member
    "E0611",  # no-name-in-module
)
PYLINT_ADDITIONAL_BUILTINS = ("__states__", "__pillar__", "__salt__", "__grains__", "__opts__", "__env__")
PYLINT_EXECUTABLE_PARAMS = (
    # parallel options
    # "-j", "4", # DOES NOT WORK: looks like drop some command line arguments
    # format options
    "--msg-template", "{path}:{line}: [{msg_id}({symbol}), {obj}] {msg}",
    # scope options
    "--additional-builtins", ",".join(PYLINT_ADDITIONAL_BUILTINS),
    "--errors-only",
    "--disable", ",".join(PYLINT_SUPPRESS_ERRORS),
)


class ImportExtension(jinja2.ext.Extension):
    tags = set(["load_yaml", "load_json", "import_yaml", "import_json", "load_text", "import_text"])

    def parse(self, parser):
        if parser.stream.current.value in ("import_yaml", "import_json", "import_text"):
            parser.parse_import()
        else:
            next(parser.stream)
            parser.stream.expect('name:as')
            parser.parse_assign_target()
            parser.parse_statements(("name:endload",), drop_needle=True)

        return None


def _filter_extensions(fnames, exts, forbid_fnames):
    filtered_fnames = []
    for ext in exts:
        m = re.compile(ext)
        filtered_fnames.extend([fname for fname in fnames if re.match(m, fname)])
    filtered_fnames = [fname for fname in filtered_fnames if fname not in forbid_fnames]

    return filtered_fnames


def validate_jinja(fnames):
    jinja_fnames = _filter_extensions(fnames, JINJA_EXTENSIONS, JINJA_BLACKLIST)
    env = jinja2.Environment(extensions = [ImportExtension, jinja2.ext.do, jinja2.ext.loopcontrols])
    for fname in jinja_fnames:
        try:
            with open(fname) as template:
                env.parse(template.read())
        except jinja2.exceptions.TemplateSyntaxError:
            print("Got syntax error while parsing {}".format(fname))
            raise


def validate_pylint(fnames):
    fnames = _filter_extensions(fnames, PYLINT_EXTENSIONS, PYLINT_BLACKLIST)

    regexes = [
        # all but <#!/usr/bin/env python3> is considered as python2 source code
        ("python2", re.compile("(?!^#!/usr/bin/env python3$)", flags=re.MULTILINE)),
        # only <#!/usr/bin/env python3> is considered as python3 source code
        ("python3", re.compile("(^#!/usr/bin/env python3$)", flags=re.MULTILINE)),
    ]

    files_by_executable = defaultdict(list)
    for fname in fnames:
        shebang_data = open(fname).read(100)
        for executable, pattern in regexes:
            if pattern.match(shebang_data):
                files_by_executable[executable].append(fname)
                break
        else:
            raise Exception("File <{}> shebang detection failed".format(fname))


    for executable in sorted(files_by_executable):
        validate_pylint_specific_version(files_by_executable[executable], executable)


def validate_pylint_specific_version(fnames, executable):
    temp_dir = tempfile.mkdtemp()
    try:
        status = subprocess.call([executable, "-m", "pylint"] + list(PYLINT_EXECUTABLE_PARAMS) + fnames)
        if status:
            raise Exception("Pylint checking exited with status <{}>".format(status))
    finally:
        try:
            shutil.rmtree(temp_dir)
        except Exception:
            pass


def main():
    if len(sys.argv) > 2:
        raise Exception("Usage: {} [<path_to_salt>]".format(sys.argv[0]))

    if sys.argv == 2:
        src_dir = sys.argv[1]
    else:
        src_dir = os.path.join(os.path.dirname(__file__), "..", "..")

    fnames = []
    for root, dirnames, filenames in os.walk(src_dir):
        fnames.extend([os.path.join(root, filename) for filename in filenames])

    # validate jinja2 templates
    validate_jinja(fnames)

    # validate python code
    validate_pylint(fnames)


if __name__ == "__main__":
    main()




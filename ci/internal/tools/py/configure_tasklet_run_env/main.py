#!/usr/bin/env python3

import os
import re
import shutil
import stat
import subprocess
from argparse import ArgumentParser
from pathlib import Path


def create_custom_python(arcadia_root):
    TEMPLATE = """#!/bin/sh
    export Y_PYTHON_SOURCE_ROOT={arcadia_root}
    export Y_PYTHON_ENTRY_POINT=":main"
    {binary} "$@"
    """

    def exit_with_error(message):
        print('Error:' + message)
        exit(1)

    make_file = Path() / 'ya.make'
    if not make_file.exists():
        exit_with_error('ya.make not found')

    text = make_file.read_text()
    res = re.search(r'PY3?_PROGRAM\((.*)\)', text)
    test_res = re.search(r'PY3?TEST\((.*)\)', text)
    if res:
        binary = res.group(1).strip(' \n')
        if len(binary) == 0:
            binary = Path().resolve().name
    elif test_res:
        binary = test_res.group(1).strip(' \n')
        if len(binary) == 0:
            p = Path().resolve()
            arcadia_index = p.parts.index('arcadia')
            if arcadia_index == -1:
                exit_with_error('Not in arcadia root')
            binary = '-'.join(p.parts[arcadia_index + 1:])
    else:
        exit_with_error('ya.make is not python program nor python tests')

    binary_path = Path().resolve() / binary

    python_file = Path() / 'python'

    with open(python_file, 'w') as p_file:
        p_file.write(TEMPLATE.format(arcadia_root=arcadia_root, binary=binary_path))

    st = os.stat(python_file)
    os.chmod(python_file, st.st_mode | stat.S_IEXEC)


if __name__ == '__main__':

    parser = ArgumentParser()
    parser.add_argument(
        "--ya-path",
        help="Ya executable path"
    )
    parser.add_argument(
        "--arcadia-path",
        help="Arcadia root folder"
    )
    parser.add_argument(
        "--python-path",
        help="Generated tasklet python path"
    )

    args = parser.parse_args()
    print(subprocess.check_output(['python2.7', args.ya_path, 'make', '--add-result', 'py']))
    create_custom_python(args.arcadia_path)
    shutil.copy('python', args.python_path)

    print('!!! Tasklet python now moved to', args.python_path)

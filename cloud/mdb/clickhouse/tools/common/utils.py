import re
import os
import subprocess
from pathlib import Path


def version_ge(version1, version2):
    """
    Return True if version1 is greater or equal than version2.
    """
    return parse_version(version1) >= parse_version(version2)


def parse_version(version):
    """
    Parse version string.
    """
    return [int(x) for x in version.strip().split('.')]


def strip_query(query_text: str) -> str:
    """
    Remove query without newlines and duplicate whitespaces.
    Copy from ch-backup/ch-backup/util.py
    """
    return re.sub(r'\s{2,}', ' ', query_text.replace('\n', ' ')).strip()


def clear_empty_directories_recursively(directory):
    try:
        directory = Path(directory)
        for item in directory.iterdir():
            if item.is_dir():
                clear_empty_directories_recursively(item)
        if len(os.listdir(directory)) == 0:
            directory.rmdir()
    except FileNotFoundError:
        print(f'Tried to remove directory {directory}, but the error arised. Maybe it was already removed.')


def execute(command):
    """
    Execute the specified command, check return code and return its output on success.
    """
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = proc.communicate()

    if proc.returncode:
        msg = '"{0}" failed with code {1}'.format(command, proc.returncode)
        if stderr:
            msg = '{0}: {1}'.format(msg, stderr.decode())

        raise RuntimeError(msg)

    return stdout.decode()

from __future__ import print_function
import os
import sys
import six
import shutil
import subprocess

import typing as tp  # noqa

from library.python.sfx.extract_program import extract_program_sources  # type: ignore


# Force fixing python stubs
GLOBAL_MODULES_TO_DELETE = [
    # Directories
    "enum",
    "library/python/sfx",  # Cannot find module named '__res'
    "library/python/resource",  # Cannot find module named '__res'
    # Files
    "six.py",  # using python stdlib
    "subprocess.py",  # Cannot find module named 'msvcrt'
    "typing.py"  # wrong stub, using python stdlib
]


def makedirs(path, exist_ok=False):
    # type: (str, bool) -> None
    """ Placeholder for Python 3 os.makedirs(path, exist_ok) """
    try:
        os.makedirs(path)
    except OSError:
        if not exist_ok or not os.path.isdir(path):
            raise


class MypyTypingError(RuntimeError):
    pass


class MypyTyping(object):
    """ Class for MyPy scanning """

    def __init__(
            self,                 # type: MypyTyping
            extraction_dir=""   # type: str
    ):  # type: (...) -> None
        self.mypy_bin = self._get_mypy_bin()
        self.extraction_dir = extraction_dir
        self._extracted = False

    @staticmethod
    def _get_mypy_bin():
        # type: (...) -> str
        import yatest  # type: ignore
        return yatest.common.binary_path("contrib/python/mypy/bin/mypy/mypy")

    def _remove_bad_modules(
            self,           # type: MypyTyping
            modules=None    # type: tp.Optional[tp.List[str]]
    ):  # type: (...) -> None
        if not self._extracted:
            self.extract_binary()
        modules = [] if modules is None else modules
        for module in GLOBAL_MODULES_TO_DELETE + modules:
            path = os.path.join(self.extraction_dir, module)
            if not os.path.exists(path):
                assert module not in modules, "Module path '{}' was passed to remove, but is wasn't found in extraction_dir.".format(module)
                continue
            if os.path.isfile(path):
                os.remove(path)
            else:
                shutil.rmtree(path)

    def extract_binary(
            self,           # type: MypyTyping
            dirname=None    # type: tp.Optional[str]
    ):  # type: (...) -> str
        self._extracted = False
        name = os.path.basename(sys.executable)
        if not self.extraction_dir:
            self.extraction_dir = 'extracted_' + name if dirname is None else dirname
        extract_program_sources(sys.executable, self.extraction_dir)
        self._extracted = True
        return self.extraction_dir

    def start_scan(
            self,                           # type: MypyTyping
            module,                         # type: str
            recursive=False,                # type: bool
            bad_modules=None,               # type: tp.Optional[tp.List[str]]
            **mypy_config                   # type: str
    ):  # type: (...) -> None
        if not self._extracted:
            self.extract_binary()

        self._remove_bad_modules(bad_modules)

        os.environ["MYPYPATH"] = os.path.abspath(os.path.join(os.curdir, self.extraction_dir))

        cmd = [self.mypy_bin]
        if six.PY2:
            cmd += ["--py2"]
        cmd += [
            "-p" if recursive else "-m",
            module
        ]

        for k, v in mypy_config.items():
            if isinstance(v, bool):
                if v is True:
                    cmd += ["--{}".format(k.replace('_', '-'))]
            else:
                cmd += ["--{}={}".format(k.replace('_', '-'), v)]

        print("\n\nRun MyPy command: mypy {}".format(' '.join(cmd[1:])), file=sys.stderr)

        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = p.communicate()
        if p.wait() != 0:
            error = (out.decode("utf-8") + '\n' + err.decode("utf-8")).replace(self.extraction_dir + '/', '')
            raise MypyTypingError(module + "\n" + error)

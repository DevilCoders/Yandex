#!/skynet/python/bin/python
"""
    Build cython modules to so files
"""

import os
import sys

import gencfg

import Cython.Build

from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

if __name__ == '__main__':
    cjson_path = os.path.join(os.path.dirname(__file__), "contrib", "cJSON")

    sys.path.append(cjson_path)

    compiler_directives = {}
    if 'CYTHON_DEBUG' in os.environ:
        compiler_directives['profile'] = True

    include_dirs = ['./contrib/cJSON']
    setup(
        ext_modules = cythonize([
            Extension(
                "hosts",
                ["./core/hosts.pyx", "./contrib/cJSON/cJSON.c"],
                include_dirs=include_dirs,
                extra_compile_args=['-std=c++11', '-march=corei7', '-g'],
                language="c++",
            ),
            Extension(
                "instances",
                ["core/instances.pyx", "./contrib/cJSON/cJSON.c"],
                include_dirs=include_dirs,
                extra_compile_args=['-std=c++11', '-march=corei7', '-g'],
                language="c++",
            ),
            Extension(
                "ghi",
                ["core/ghi.pyx", "./contrib/cJSON/cJSON.c"],
                include_dirs=include_dirs,
                extra_compile_args=['-std=c++11', '-march=corei7', '-g'],
                language="c++",
            ),
            Extension(
                "intlookups",
                ["core/intlookups.pyx", "./contrib/cJSON/cJSON.c"],
                include_dirs=include_dirs,
                extra_compile_args=['-std=c++11', '-march=corei7', '-g'],
                language="c++",
            )],
            compiler_directives=compiler_directives,
        )
    )

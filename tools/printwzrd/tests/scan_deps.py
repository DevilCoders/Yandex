#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Тест для отслеживания зависимостей для программы tools/printwzrd.

Для работы требует файл cmake_deps, который можно сгенерировать
локально, если создать переменную окружения: CALC_CMAKE_DEPS=yes
и запустить cmake
"""

import imp
from os.path import abspath, join, relpath, dirname, exists
from os import getpid
import os
import sys
import traceback

from simple_tests_common import *
from tests_common import source_root_path, do_test, Subtest
import tests_common

TEST_UNIT = "tools/printwzrd"

DEPS_COLLECTOR_RELATIVE_PATH = "check/deps_collector.py"
OUTPUT_FILENAME              = "printwzrd_deps.txt.out"

EXCEPTIONS = set(["util"])

def find_cmake_deps(path, offset):
    cmake_deps_filename = join(dirname(dirname(dirname(path))),
                               "cmake_deps")

    tests_common.accept_subtest_status_on_exit("cmake_deps file search",
                                               exists(cmake_deps_filename),
                                               offset,
                                               ex_code = tests_common.bin_nfnd_ex_code)

tests_common.make_subtest_find_binary = find_cmake_deps

def not_exception(path):
    for exception in EXCEPTIONS:
        if path.startswith(exception):
            return False
    return True

class ScanDependenciesDataProcessor(object):
    def __init__(self, main_paths, input_path, output_path, all_files, timeout):
        self._source_root_path = source_root_path()
        self._binary_root_path = main_paths.binary_root_path()
        self._program_path     = dirname(relpath(main_paths.binary_path(), self._binary_root_path))

        self.cmake_deps_filename = join(self._binary_root_path, "cmake_deps")

    def process(self, receiver, exec_groups_indices):
        deps_collector = imp.load_source("deps_collector",
                                         join(self._source_root_path,
                                              DEPS_COLLECTOR_RELATIVE_PATH))

        cmake_deps_filename = self.cmake_deps_filename

        try:
            receiver.open_session(OUTPUT_FILENAME, 0)

            dependencies_collector = deps_collector.DependenciesCollector(cmake_deps_filename)

            parent_targets = filter(not_exception, dependencies_collector.GetDepandenciesForUnit(TEST_UNIT))
            parent_targets.sort()

            for line in parent_targets:
                receiver.process_line(line)
                receiver.process_line("\n")
            is_ok = True
        except:
            traceback.print_exc(file = sys.stdout)
            is_ok = False

        receiver.close_session(is_ok, getpid())

    def exec_groups_count(self):
        return 1

    def clean_up(self):
        return

if __name__ == '__main__':
    try:
        subtest = Subtest(test_data_processor_factory=ScanDependenciesDataProcessor,
                          input_path = "",
                          timeout    = 60,
                          run_desc   = "dependency scanner test",
                          dir_desc   = "printwzrd_deps")

        do_test([subtest])
    except Exception, e:
        traceback.print_exc(file = sys.stdout)
        traceback.print_stack()
        raise

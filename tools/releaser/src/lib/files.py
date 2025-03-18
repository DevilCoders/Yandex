# coding: utf-8

from __future__ import unicode_literals

import os


def find_first_existing_file(path_list):
    for path in path_list:
        if os.path.exists(path):
            return path

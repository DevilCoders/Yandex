#!/usr/bin/env python
import os
from simple_tests_common import *

def inputfile(fname):
    return os.path.join(get_arcadia_tests_data(), 'structhtml', 'domtree', fname)

def options():
    return lambda fname, cfg: ['-l', fname, '-m', 'treetext']

do_test([
    create_subtest(
        inputfile('list.txt'), 
        options(), 
        100,
        opt_desc_arg = 'DomTree_TreeText',
        dir_desc_arg = 'DomTree_TreeText'),
])

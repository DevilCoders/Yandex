#!/usr/bin/env python
import fnmatch
import os
import sys


assert(len(sys.argv) > 1)

templates_path, matches = sys.argv[1], sys.argv[2:]

print """#include "tpl/cache.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace tpl {

TplCache::TplCache()
{
"""

for fname in matches:
    print '    add("/' + os.path.relpath(fname, templates_path) + '",'
    with open(fname, 'r') as f:
        empty = True
        for line in f:
            empty = False
            print '        "{}"'.format(line.replace('\\', '\\\\').replace('\"', '\\\"').replace('\n', '\\n'))
        if empty:
            print '        ""'
    print '    );\n'

print """    Freeze();
}

} // namespace tpl
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
"""

import errno
import os
import stat

import exts.path2 as pathx


def affected_paths(base_paths, wc_root):
    result = []
    for p in base_paths:
        result.append(p)
        for parent in pathx.path_prefixes(p):
            if not os.path.exists(os.path.join(wc_root, parent)):
                result.append(parent)
    return list(set(result))


def check_rigths(wc_root, affected_paths, write_access=True):
    write_mode = stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH
    for p in affected_paths:
        path = os.path.join(wc_root, p)
        try:
            assert write_access == bool(os.stat(path).st_mode & write_mode), \
                'Write permission is {} for path {}'.format('unset' if write_access else 'set', p)
            if os.path.isdir(path):
                assert is_executable(path), 'Access permission is unset for directory {}'.format(path)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise


def is_executable(path):
    return os.stat(path).st_mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

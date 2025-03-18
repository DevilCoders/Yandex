"""
Save python source code on the filesystem in a proper hierarchy.

This code must have no dependencies to be importable into any Arcadia Python program.
"""

import json
import os
import sys

import six

import __res as resource


def create_parents(path, mode='w'):
    dir = os.path.dirname(path)
    if not os.path.exists(dir):
        os.makedirs(dir)
    return path


def create_inits(path):
    for dirpath, dirnames, filenames in os.walk(path):
        if dirpath != path and '__init__.py' not in filenames:
            open(os.path.join(dirpath, '__init__.py'), 'w').close()


def iter_py_fs():
    """
    For each incorporated module yield `(source_path, import_name)`.
    """
    for mod in resource.iter_py_modules():
        rel_src_path = resource.importer.get_filename(mod)
        yield rel_src_path, mod


def iter_by_py_modules(keep_src_path=False, keep_python3_std_library=False):
    for mod in resource.iter_py_modules():
        rel_src_path = resource.importer.get_filename(mod)

        if not keep_python3_std_library and rel_src_path.startswith('contrib/tools/python3'):
            continue

        if keep_src_path:
            if rel_src_path.startswith('/-B/'):
                rel_src_path = rel_src_path[4:]
            if rel_src_path.startswith('/'):
                raise RuntimeError("src path startswith '/': path = {}".format(rel_src_path))
            rel_out_path = rel_src_path
        else:
            rel_out_path = mod.replace('.', os.path.sep) + '.py'

        data = six.ensure_binary(resource.importer.get_source(mod))

        yield rel_out_path, rel_src_path, data


def iter_by_other_resources(contrib_package_prefixes):
    for rel_out_path in resource.resfs_files():
        # Do not extract .py files twice.
        if six.PY3 and rel_out_path.startswith(resource.py_prefix):
            continue

        rel_src_path = six.ensure_str(resource.resfs_src(rel_out_path, resfs_file=True))
        data = resource.resfs_read(rel_out_path, builtin=True)

        rel_out_path = six.ensure_str(rel_out_path)
        if rel_out_path.startswith("contrib/") and "/.dist-info/" not in rel_out_path:
            for prefix in contrib_package_prefixes:
                if rel_out_path.startswith(prefix):
                    rel_out_path = rel_out_path[len(prefix):]

        yield rel_out_path, rel_src_path, data


def extract_sources(out_dir, sym_source_root=None, keep_src_path=False, keep_python3_std_library=False):
    """
    Save sources in out_dir.
    With sym_source_root, link to sources in sym_source_root (source root).
    Return a map from extracted paths (relative to out_dir) to source paths
    (relative to source root).
    Also save this map in out_dir/source_map.json.
    """
    source_map = {}
    contrib_package_prefixes = set()

    def extract_file(rel_out_path, rel_src_path, data):
        source_map[rel_out_path] = rel_src_path
        out_path = create_parents(os.path.join(out_dir, rel_out_path))

        if os.path.exists(out_path):
            os.unlink(out_path)

        src_path = sym_source_root and os.path.join(sym_source_root, rel_src_path)
        if src_path and os.path.exists(src_path):
            os.symlink(src_path, out_path)
        else:
            with open(out_path, 'wb') as f:
                f.write(data)

    # Do not use sys.extra_modules as it contains fictional __init__.py's.
    for rel_out_path, rel_src_path, data in iter_by_py_modules(keep_src_path, keep_python3_std_library):
        if rel_src_path.startswith("contrib/"):
            contrib_package_prefixes.add(rel_src_path[:-len(rel_out_path)])
        extract_file(rel_out_path, rel_src_path, data)

    contrib_package_prefixes.discard("")
    create_inits(out_dir)

    # Extract other resources.
    for rel_out_path, rel_src_path, data in iter_by_other_resources(contrib_package_prefixes):
        extract_file(rel_out_path, rel_src_path, data)

    with open(os.path.join(out_dir, 'source_map.json'), 'w') as f:
        json.dump(source_map, f)

    return source_map


def main():
    """
    This main is invoked from extract_program_sources.
    """
    extract_sources(*sys.argv[1:])

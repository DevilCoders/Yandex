#!/usr/bin/env python
import json
import os
import shutil
import exts.archive


# FIXME: This is ridicules but because of https://st.yandex-team.ru/DEVTOOLS-5105
# the only way to get package's inside VM make is to make it manualy by
# copy-pasting package's hierarchy here.

def ignore_symlink(path, names):
    ignore = []
    for name in names:
        if os.path.islink(os.path.join(path, name)):
            ignore.append(ignore)
    return ignore


class FakeYaPackage(object):
    def __init__(self, spec_file, src_fn, bin_fn, tmp_root):
        self._spec_file = spec_file
        self._pkg_root = os.path.dirname(spec_file)
        self._src_fn = src_fn
        self._bin_fn = bin_fn
        self._tmp_root = tmp_root

        assert spec_file.endswith('.json')
        with open(spec_file) as f:
            self._spec = json.load(f)
            assert self._spec['data']

    def _do_copy(self, src, dst, is_symlink=False):
        if os.path.isabs(dst):
            full_dst = self._tmp_root + dst
        else:
            full_dst = os.path.join(self._tmp_root, dst)

        if not os.path.exists(os.path.dirname(full_dst)):
            os.makedirs(os.path.dirname(full_dst))

        if is_symlink:
            return os.symlink(src, full_dst)
        if os.path.isdir(src):
            return shutil.copytree(src, full_dst, ignore=ignore_symlink)
        # Regual file
        shutil.copy2(src, full_dst)

    def _do_one_entry(self, entry):
        if entry['source']['type'] == 'BUILD_OUTPUT':
            self._do_copy(self._bin_fn(entry['source']['path']), entry['destination']['path'])
            return
        if entry['source']['type'] == 'ARCADIA':
            self._do_copy(self._src_fn(entry['source']['path']), entry['destination']['path'])
            return
        if entry['source']['type'] == 'RELATIVE':
            self._do_copy(self._src_fn(os.path.join(self._pkg_root, entry['source']['path'])),
                          entry['destination']['path'])
            return
        if entry['source']['type'] == 'SYMLINK':
            self._do_copy(entry['destination']['target'], entry['destination']['path'], True)
            return
        raise Exception("Type %s support is not implemented for FakeYaPackage" % entry['source']['type'])

    def _do_prep_content(self):
        for e in self._spec['data']:
            self._do_one_entry(e)

    def make_tarball(self, tar_file):
        print("Do make_tarball %s" % tar_file)
        assert tar_file.endswith('.tar')
        self._do_prep_content()
        exts.archive.create_tar(self._tmp_root, tar_file, None, fixed_mtime=None)

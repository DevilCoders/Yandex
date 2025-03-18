import os
import pytest
import yatest.common
from slugify import slugify

SBCTL_BIN = yatest.common.binary_path('tools/sandboxctl/bin/sandboxctl')
DEF_TASK = 112561093
DEF_RESOURCE = 265669912


class TestGuestCmd(object):
    FORMAT_LIST = ['-q', '--oneline', '--manifest']

    def bin_exec(self, cmd):
        run_cmd = [SBCTL_BIN] + cmd
        yatest.common.execute(run_cmd)

    def canon_exec(self, cmd, name=None):
        if name is None:
            name = slugify(str(cmd))
            print("name: %s" % name)
        path = yatest.common.output_path(name + ".stdout")
        print ('out_path %s' % yatest.common.output_path())
        print ('path %s' % path)
        run_cmd = [SBCTL_BIN] + cmd
        with open(path, "w") as fout:
            yatest.common.execute(run_cmd, stdout=fout)
        return yatest.common.canonical_file(path)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_get_task(self, format_opt):
        cmd = ['get_task', str(DEF_TASK), format_opt]
        return self.canon_exec(cmd)

    def test_get_task_full(self):
        cmd = ['get_task', str(DEF_TASK), '--full']
        return self.bin_exec(cmd)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_get_resource(self, format_opt):
        res_root = slugify(format_opt)
        os.makedirs(res_root)
        cmd = ['get_resource', str(DEF_RESOURCE), format_opt, '--out-root', res_root]
        return self.canon_exec(cmd)

    def test_get_resource_full(self):
        res_root = yatest.common.work_path('res_root_full')
        os.makedirs(res_root)
        cmd = ['get_resource', str(DEF_RESOURCE), '-O', res_root, '--full']
        return self.bin_exec(cmd)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_list_resource_by_id(self, format_opt):
        cmd = ['list_resource', '--id', str(DEF_RESOURCE), format_opt]
        return self.canon_exec(cmd)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST + ['--full'])
    def test_list_resource_by_type(self, format_opt):
        cmd = ['list_resource', '--type', 'YA_PACKAGE', format_opt]
        self.bin_exec(cmd)

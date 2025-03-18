import pytest
import yatest.common
from slugify import slugify

SBCTL_BIN = yatest.common.binary_path('tools/sandboxctl/bin/sandboxctl')
DEF_TASK = 112561093
DEF_RESOURCE = 265669912


class TestDryRunCmd(object):
    FORMAT_LIST = ['', '-q', '--oneline', '--manifest']

    def bin_exec(self, cmd):
        run_cmd = [SBCTL_BIN] + cmd + ['--dry-run']
        yatest.common.execute(run_cmd)

    def canon_exec(self, cmd, name=None):
        if name is None:
            name = slugify(str(cmd))
            print("name: %s" % name)
        path = yatest.common.output_path(name + ".stdout")
        print ('out_path %s' % yatest.common.output_path())
        print ('path %s' % path)
        run_cmd = [SBCTL_BIN] + cmd + ['--dry-run']
        with open(path, "w") as fout:
            yatest.common.execute(run_cmd, stdout=fout)
        return yatest.common.canonical_file(path)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_ya_make_art(self, format_opt):
        cmd = ['ya-make', 'infra/qemu/vmexec/vmexec',  'tools/sandboxctl/bin/sandboxctl', format_opt]
        return self.canon_exec(cmd)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_ya_make_art_single(self, format_opt):
        cmd = ['ya-make', '--single', 'infra/qemu/vmexec/vmexec',  'tools/sandboxctl/bin/sandboxctl', format_opt]
        return self.canon_exec(cmd)

    @pytest.mark.parametrize('format_opt', FORMAT_LIST)
    def test_ya_make_tgt(self, format_opt):
        cmd = ['ya-make', '--target', 'infra/qemu/vmexec/', '--target', 'tools/sandboxctl/bin', format_opt]
        return self.canon_exec(cmd)

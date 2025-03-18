import yatest
import subprocess

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite


CBB_FLAG = 1
BLOCK_RULES = (
    r'header["host"]=/bad-host\..+/',
    r'ip=1.2.3.4/30',
    r'doc=/.*\/yandsearch1/',
)
DUMMY_RULE = "doc=/test/"


class TestAntiDDosStandalone(AntirobotTestSuite):
    @classmethod
    def setup_class(cls):
        super().setup_class()
        cls.bin_path = yatest.common.build_path("antirobot/tools/antiddos/antiddos")

    def popen(self, args):
        return subprocess.Popen([self.bin_path] + args,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

    def test_parse_command_ok(self):
        p = self.popen(['parse', '-'])
        (stdout, err) = p.communicate(input='\n'.join(BLOCK_RULES).encode())
        assert p.returncode == 0
        assert stdout.strip() == b"Parsed OK!"

    def test_parse_command_fail(self):
        p = self.popen(['parse', '-'])
        (stdout, err) = p.communicate(input=b'doc=')
        assert p.returncode != 0

    def test_store_command(self):
        self.cbb.add_text_block(CBB_FLAG, DUMMY_RULE)

        p = self.popen(['--cbb-host', self.cbb.host, '--cbb-flag', str(CBB_FLAG), 'store', '-'])
        (stdout, err) = p.communicate(input='\n'.join(BLOCK_RULES).encode())

        assert p.returncode == 0
        resp = self.cbb.fetch_flag_data(CBB_FLAG, with_format=["range_txt"])
        assert resp.getcode() == 200

        flag_data = resp.readlines()
        assert len(flag_data) == len(BLOCK_RULES) + 1

    def test_store_command_with_clear(self):
        self.cbb.add_text_block(CBB_FLAG, DUMMY_RULE)

        p = self.popen(['--clear', '--cbb-host', self.cbb.host, '--cbb-flag', str(CBB_FLAG), 'store', '-'])
        (stdout, err) = p.communicate(input='\n'.join(BLOCK_RULES).encode())

        assert p.returncode == 0
        resp = self.cbb.fetch_flag_data(CBB_FLAG, with_format=["range_txt"])
        assert resp.getcode() == 200

        flag_data = resp.readlines()
        assert len(flag_data) == len(BLOCK_RULES)

import tempfile
import os
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.arcadia_test.util.asserts import AssertEventuallyTrue
from antirobot.daemon.arcadia_test.util import (
    GenRandomIP,
    ProtoListIO,
)
from antirobot.idl import cache_sync_pb2


AMNESTY_IP_INTERVAL = 3600
CHECKPOINT_SAVE_PERIOD = 1
SAVING_LAG = 1


def count_proto(path):
    return len(ProtoListIO(cache_sync_pb2.TBanAction).read(path))


def count_lines(path):
    with open(path) as file:
        return sum(1 for _ in file)


class TestCheckpoints(AntirobotTestSuite):
    num_antirobots = 0
    num_old_antirobots = 0
    options = {
        "AmnestyIpInterval": AMNESTY_IP_INTERVAL,
        "RemoveExpiredPeriod": AMNESTY_IP_INTERVAL / 2,
        "RobotUidsDumpPeriod": CHECKPOINT_SAVE_PERIOD,
    }

    def test_creates_in_runtime_reads_at_startup(self):
        with tempfile.TemporaryDirectory() as work_dir:
            options = dict(self.options,
                           RobotUidsDumpFile=os.path.join(Path(work_dir).absolute(), "robots"),
                           BlocksDumpFile=os.path.join(Path(work_dir).absolute(), "blocks"))

            with self.start_antirobots(options, num_antirobots=1) as antirobots:
                d = antirobots[0]
                checkpoint_files = [
                    (d.dump_cfg()["RobotUidsDumpFile"], count_proto),
                    (d.dump_cfg()["BlocksDumpFile"], count_lines),
                ]

                ban_ips = list(set([GenRandomIP() for _ in range(5)]))
                block_ips = list(set([GenRandomIP() for _ in range(12)]))

                for ip in ban_ips:
                    d.ban(ip)
                for ip in block_ips:
                    d.block(ip, duration=9999)

                for ips, (file, count) in zip([ban_ips, block_ips], checkpoint_files):
                    AssertEventuallyTrue(lambda: os.path.exists(file))
                    AssertEventuallyTrue(lambda: count(file) >= len(ips))

            with self.start_antirobots(options, num_antirobots=1) as antirobots:
                d = antirobots[0]
                for ip in ban_ips:
                    assert d.is_banned(ip)
                for ip in block_ips:
                    assert d.is_blocked(ip)

    def test_starts_normally_if_checkpoints_are_corrupted(self):
        with tempfile.TemporaryDirectory() as work_dir:
            options = dict(self.options,
                           RobotUidsDumpFile=os.path.join(Path(work_dir).absolute(), "robots"),
                           BlocksDumpFile=os.path.join(Path(work_dir).absolute(), "blocks"))
            for option in ['RobotUidsDumpFile', 'BlocksDumpFile']:
                with open(options[option], "wt") as checkpoint:
                    checkpoint.write("upyachka")

            with self.start_antirobots(options, num_antirobots=1) as antirobots:
                d = antirobots[0]
                assert d.is_alive()
                d.ban(GenRandomIP())

    def test_robots_checkpoint_is_cleared_after_amnesty(self):
        with tempfile.TemporaryDirectory() as work_dir:
            checkpoint_file = os.path.join(Path(work_dir).absolute(), "robots")
            options = dict(self.options,
                           RobotUidsDumpFile=checkpoint_file,
                           RobotUidsDumpPeriod=5)

            with self.start_antirobots(options, num_antirobots=1) as antirobots:
                d = antirobots[0]
                d.ban(GenRandomIP())
                AssertEventuallyTrue(lambda: os.path.exists(checkpoint_file))
                AssertEventuallyTrue(lambda: os.path.getsize(checkpoint_file) > 0)
                d.amnesty()
            # HACK: в тесте нужно убедиться, что файл обнулила именно амнистия, а не обычный дампер через интервал
            # поэтому этот интервал выставлен больше 1 секунды, чтобы _точно_ не успело сдампить
            assert os.path.getsize(checkpoint_file) == 0

    def test_flushes_at_shutdown(self):
        with tempfile.TemporaryDirectory() as work_dir:
            options = dict(self.options,
                           RobotUidsDumpFile=os.path.join(Path(work_dir).absolute(), "robots"),
                           BlocksDumpFile=os.path.join(Path(work_dir).absolute(), "blocks"),
                           RobotUidsDumpPeriod=9999)

            with self.start_antirobots(options, num_antirobots=1) as antirobots:
                d = antirobots[0]

                checkpoint_files = [
                    (d.dump_cfg()["RobotUidsDumpFile"], count_proto),
                    (d.dump_cfg()["BlocksDumpFile"], count_lines),
                ]

                ban_ips = list(set([GenRandomIP() for _ in range(5)]))
                block_ips = list(set([GenRandomIP() for _ in range(12)]))

                for ip in ban_ips:
                    d.ban(ip)
                for ip in block_ips:
                    d.block(ip)
                for file, _ in checkpoint_files:
                    assert not os.path.exists(file)

            for ips, (file, count) in zip([ban_ips, block_ips], checkpoint_files):
                assert os.path.exists(file)
                assert count(file) >= len(ips)

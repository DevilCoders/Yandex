import pytest

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import AntirobotTestSuite, Antirobot
from antirobot.daemon.arcadia_test.util.asserts import AssertEventuallyTrue


class TestCollapsingIPv6(AntirobotTestSuite):
    options = {
        "CbbEnabled": 1,
    }

    def test_ban_whole_subnet(self):
        self.antirobot.ban("2a02:6b8:408:0:21b0:4e37:5835:2d29")

        assert self.antirobot.is_banned("2a02:6b8:408:0:21b0:4e37:5835:2d29")
        assert self.antirobot.is_banned("2a02:6b8:408:0:21b1:4e37:5835:2d29")
        assert not self.antirobot.is_banned("2a02:6b8:408:1:21b0:4e37:5835:2d29")

    def test_block_whole_subnet(self):
        self.antirobot.block("2a02:6b8:408:0:21b0:4e37:5835:2d29", 1)

        assert self.antirobot.is_blocked("2a02:6b8:408:0:21b0:4e37:5835:2d29")
        assert self.antirobot.is_blocked("2a02:6b8:408:0:21b1:4e37:5835:2d29")
        assert not self.antirobot.is_blocked("2a02:6b8:408:1:21b0:4e37:5835:2d29")


class TestCollapsingIPv6Pair(AntirobotTestSuite):
    num_antirobots = 0

    @pytest.mark.parametrize("where", ["cacher", "processor"])
    def test_ban_when_different_number_of_bits_to_collapse(self, where):
        """
        Баним IP на кэшере.
        Процессор должен забанить подсеть соответвующую этому IP и своему значению CollapsedIP6SubnetBits.
        И не должен прибанить ничего лишнего.
        """
        ports = self.get_ports(2)
        process_ports = self.get_ports(2)
        admin_ports = self.get_ports(2)
        unistat_ports = self.get_ports(2)

        cacher_options = {
            "Port": ports[0],
            "ProcessServerPort": process_ports[0],
            "AllDaemons": f"localhost:{process_ports[1]}",
            "AdminServerPort": admin_ports[0],
            "UnistatServerPort": unistat_ports[0],
        }

        processor_options = {
            "Port": ports[1],
            "ProcessServerPort": process_ports[1],
            "AllDaemons": f"localhost:{process_ports[1]}",
            "AdminServerPort": admin_ports[1],
            "UnistatServerPort": unistat_ports[1],
        }

        with \
                Antirobot.make(cacher_options, parent_context=self) as cacher, \
                Antirobot.make(processor_options, parent_context=self) as processor:
            cacher.wait()
            processor.wait()

            assert where in ("cacher", "processor")

            if where == "cacher":
                sender = cacher
                receiver = processor
            elif where == "processor":
                sender = processor
                receiver = cacher

            sender.ban("2a02:6b8:408:0:21b0:4e37:5835:2d29")
            AssertEventuallyTrue(lambda: receiver.is_banned("2a02:6b8:408:0:21b0:4e37:5835:2d29"))
            assert receiver.is_banned("2a02:6b8:408:0:21b1:4e37:5835:2d29")
            assert not receiver.is_banned("2a02:6b8:408:1:21b0:4e37:5835:2d29")

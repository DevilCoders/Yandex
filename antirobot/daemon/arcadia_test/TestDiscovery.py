from pathlib import Path
import time

import infra.yp_service_discovery.api.api_pb2

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import (
    Antirobot,
    AntirobotTestSuite,
    Discovery
)
from antirobot.daemon.arcadia_test.util import asserts


DISCOVERY_FREQ = 1
WATCHER_PERIOD = 1


def make_discovery_pair(cluster, endpoint_set_id, id_to_port):
    req = infra.yp_service_discovery.api.api_pb2.TReqResolveEndpoints()
    req.cluster_name = cluster
    req.endpoint_set_id = endpoint_set_id

    rsp = infra.yp_service_discovery.api.api_pb2.TRspResolveEndpoints()

    for ident, port in id_to_port.items():
        endpoint = rsp.endpoint_set.endpoints.add()
        endpoint.id = ident
        endpoint.protocol = "tcp"
        endpoint.fqdn = "localhost"
        endpoint.ip4_address = "127.0.0.1"
        endpoint.port = port
        endpoint.ready = True

    return req, rsp


def wait_for_discovery_update(daemon):
    old_discovery_updates = daemon.query_metric("discovery_updates_deee")
    time.sleep(DISCOVERY_FREQ)

    asserts.AssertEventuallyTrue(
        lambda: daemon.query_metric("discovery_updates_deee") > old_discovery_updates,
    )


# This test ensures that Antirobot:
#  1) manages to start without discovery;
#  2) pulls data from discovery once it starts;
#  3) doesn't lose old routes when discovery dies again.
class TestDiscovery1(AntirobotTestSuite):
    options = {
        "DiscoveryHost": "localhost",
        "DiscoveryFrequency": f"{DISCOVERY_FREQ}s",
        "AllDaemons": "yp:vla:antirobot-vla",
    }

    num_antirobots = 2

    @classmethod
    def setup_subclass_before_antirobots(cls):
        cls.__discovery_port = cls.get_port()
        cls.__discovery_admin_port = cls.get_port()
        cls.options["DiscoveryPort"] = cls.__discovery_port

    def test_discovery(self):
        assert self.antirobots[0].query_metric("discovery_status_ahhh") == 0

        self.__discovery = self.enter(Discovery(
            self.__discovery_port,
            self.__discovery_admin_port,
            mute=self.mute,
        ))

        self.__discovery.wait()

        self.__discovery.set(*make_discovery_pair(
            "vla", "antirobot-vla",
            {"1": self.antirobots[1].process_port},
        ))

        def ping():
            old_requests = self.antirobots[1].query_metric("requests_deee")

            self.antirobots[0].ping_search()

            asserts.AssertEventuallyTrue(
                lambda: self.antirobots[1].query_metric("requests_deee") > old_requests,
            )

        wait_for_discovery_update(self.antirobots[0])
        assert self.antirobots[0].query_metric("discovery_status_ahhh") == 1
        ping()

        self.__discovery.terminate()
        ping()


class TestDiscoveryDisabling(AntirobotTestSuite):
    controls = Path.cwd() / "controls"
    stop_discovery_for_all = controls / "stop_discovery_for_all"

    num_antirobots = 0
    discovery_args = []

    def test_discovery_disabling(self):
        self.controls.mkdir(exist_ok=True)

        (
            cacher_port, cacher_process_port, cacher_admin_port, cacher_unistat_port,
            processor1_port, processor1_process_port, processor1_admin_port, processor1_unistat_port,
            processor2_port, processor2_process_port, processor2_admin_port, processor2_unistat_port,
        ) = self.get_ports(12)

        cacher_options = {
            "Port": cacher_port,
            "ProcessServerPort": cacher_process_port,
            "AdminServerPort": cacher_admin_port,
            "UnistatServerPort": cacher_unistat_port,
            "DiscoveryHost": "localhost",
            "DiscoveryPort": self.discovery.port,
            "DiscoveryFrequency": DISCOVERY_FREQ,
            "AllDaemons": f"yp:vla:antirobot-vla localhost:{processor1_process_port}",
            "HandleWatcherPollInterval": WATCHER_PERIOD,
            "HandleStopDiscoveryForAllFilePath": self.stop_discovery_for_all,
        }

        processor1_options = {
            "Port": processor1_port,
            "ProcessServerPort": processor1_process_port,
            "AdminServerPort": processor1_admin_port,
            "UnistatServerPort": processor1_unistat_port,
            "AllDaemons": f"localhost:{processor1_process_port}",
        }

        processor2_options = {
            "Port": processor2_port,
            "ProcessServerPort": processor2_process_port,
            "AdminServerPort": processor2_admin_port,
            "UnistatServerPort": processor2_unistat_port,
            "AllDaemons": f"localhost:{processor2_process_port}",
        }

        self.cacher = self.enter(Antirobot.make(
            cacher_options,
            parent_context=self,
        ))

        self.processor1 = self.enter(Antirobot.make(
            processor1_options,
            parent_context=self,
        ))

        self.processor2 = self.enter(Antirobot.make(
            processor2_options,
            parent_context=self,
        ))

        self.cacher.wait()
        self.processor1.wait()
        self.processor2.wait()

        self.discovery.set(*make_discovery_pair(
            "vla", "antirobot-vla",
            {"1": self.processor2.process_port},
        ))

        wait_for_discovery_update(self.cacher)

        old_requests = self.processor2.query_metric("requests_deee")
        self.cacher.ping_search()

        asserts.AssertEventuallyTrue(
            lambda: self.processor2.query_metric("requests_deee") > old_requests,
        )

        with open(self.stop_discovery_for_all, "w") as file:
            file.write("enable")

        time.sleep(WATCHER_PERIOD + 1)

        old_requests = self.processor1.query_metric("requests_deee")
        self.cacher.ping_search()

        asserts.AssertEventuallyTrue(
            lambda: self.processor1.query_metric("requests_deee") > old_requests,
        )

import time
from pathlib import Path

from antirobot.daemon.arcadia_test.util.AntirobotTestSuite import (
    Antirobot,
    AntirobotTestSuite,
)

from antirobot.daemon.arcadia_test import util


WATCHER_PERIOD = 1


class TestYqlRules(AntirobotTestSuite):
    controls = Path.cwd() / "controls"
    stop_yql = controls / "stop_yql"

    num_antirobots = 0

    captcha_args = ["--generate", "correct", "--check", "success"]
    wizard_args = []

    @classmethod
    def setup_subclass(cls):
        cls.controls.mkdir(exist_ok=True)

        (
            cacher_port, cacher_process_port, cacher_admin_port, cacher_unistat_port,
            processor_port, processor_process_port, processor_admin_port, processor_unistat_port
        ) = cls.get_ports(8)

        cacher_options = {
            "Port": cacher_port,
            "ProcessServerPort": cacher_process_port,
            "AdminServerPort": cacher_admin_port,
            "UnistatServerPort": cacher_unistat_port,
            "AllDaemons": f"localhost:{processor_process_port}",
            "threshold": 99,
        }

        processor_options = {
            "Port": processor_port,
            "ProcessServerPort": processor_process_port,
            "AdminServerPort": processor_admin_port,
            "UnistatServerPort": processor_unistat_port,
            "AllDaemons": f"localhost:{processor_process_port}",
            "threshold": 99,
            "HandleWatcherPollInterval": WATCHER_PERIOD,
            "HandleStopYqlFilePath": cls.stop_yql,
        }

        cls.cacher = cls.enter(Antirobot.make(
            cacher_options,
            parent_context=cls,
        ))

        cls.processor = cls.enter(Antirobot.make(
            processor_options,
            parent_context=cls,
            global_config={
                "rules": [
                    {"id": 777, "cbb": [], "yql": ["FALSE"]},
                    {"id": 444, "cbb": ["service_type=/web/"], "yql": ["TRUE", "has_cookie_my == 1"]},
                    {"id": 333, "cbb": [], "yql": ["has_cookie_YX_SHOW_RELEVANCE == 1"]},
                    {"id": 555, "cbb": ["service_type=/morda/"], "yql": ["FALSE", "FALSE", "FALSE"]},
                ],
                "mark_rules": [
                    {"id": 1, "cbb": [], "yql": ["has_cookie_L == 1"]},
                ],
            },
        ))

        cls.cacher.wait()
        cls.processor.wait()

    def setup_method(self, method):
        super().setup_method(method)

        with open(self.stop_yql, "w") as stop_yql_file:
            stop_yql_file.write("")

    def test_yql_rules(self):
        ip = util.GenRandomIP()

        def ping(cookie):
            self.cacher.send_fullreq(
                "https://yandex.ru/search?text=woof",
                headers={
                    "X-Forwarded-For-Y": ip,
                    "Cookie": f"{cookie}=1234",
                },
            )

        ping("my")
        time.sleep(3)

        events = self.unified_agent.pop_daemon_logs()
        need_event = None

        for event in events:
            if "woof" in event["req_url"] and "rules" in event:
                need_event = event

        assert need_event is not None
        assert need_event["rules"] == ["444"]

        ping("YX_SHOW_RELEVANCE")
        time.sleep(3)

        events = self.unified_agent.pop_daemon_logs()
        need_event = None

        for event in events:
            if "woof" in event["req_url"] and "rules" in event:
                need_event = event

        assert need_event is not None
        assert need_event["prev_rules"] == ["444"]
        assert need_event["rules"] == ["333"]

    def test_yql_mark_rules(self):
        ip = util.GenRandomIP()

        self.cacher.send_fullreq(
            "https://yandex.ru/search?text=woof",
            headers={
                "X-Forwarded-For-Y": ip,
                "Cookie": "L=1234",
            },
        )
        time.sleep(3)

        events = self.unified_agent.pop_daemon_logs()
        need_event = None

        for event in events:
            if "woof" in event["req_url"] and "mark_rules" in event:
                need_event = event

        assert need_event is not None
        assert need_event["mark_rules"] == ["1"]

    def test_yql_rule_disabling(self):
        ip = util.GenRandomIP()

        old_metric = self.processor.query_metric("robots_by_yql_deee")

        with open(self.stop_yql, "w") as stop_yql_file:
            stop_yql_file.write("web")

        time.sleep(WATCHER_PERIOD + 1)

        self.cacher.send_fullreq(
            "https://yandex.ru/search?text=woof",
            headers={
                "X-Forwarded-For-Y": ip,
                "Cookie": "my=1234",
            },
        )
        time.sleep(3)

        events = self.unified_agent.pop_daemon_logs()
        was_need_event = False

        for event in events:
            if "woof" in event["req_url"]:
                assert event.get("rules", []) == []
                was_need_event = True

        assert was_need_event is True

        new_metric = self.processor.query_metric("robots_by_yql_deee")
        assert new_metric == old_metric

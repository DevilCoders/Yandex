from prometheus_metrics.event import EventConnoisseur, EventFinger


class TestEventConnoisseur_get_event:  # noqa
    connoisseur = EventConnoisseur()

    def test_for_non_dict_event(self):
        assert self.connoisseur.get_finger({}) == EventFinger()

    def test_minion_start(self):
        data = {
            "tag": "minion_start",
            "data": {
                "_stamp": "2021-07-13T15:58:45.333958",
                "pretag": None,
                "cmd": "_minion_event",
                "tag": "minion_start",
                "data": "Minion sas-049296fmotw8020v.db.yandex.net started at Tue Jul 13 18:58:45 2021",
                "id": "sas-049296fmotw8020v.db.yandex.net",
            },
        }
        assert self.connoisseur.get_finger(data).tag == "minion_start"

    def test_key(self):
        data = {
            "tag": "salt/key",
            "data": {
                "_stamp": "2021-07-13T15:58:34.525606",
                "act": "accept",
                "id": "sas-049296fmotw8020v.db.yandex.net",
                "result": True,
            },
        }

        assert self.connoisseur.get_finger(data).tag == "key_accept"

    def test_ignored_event(self):
        data = {
            "tag": "20210713161700143379",
            "data": {
                "_stamp": "2021-07-13T16:17:00.143650",
                "minions": ["wizard-internal-api-test-01i.db.yandex.net"],
            },
        }
        assert self.connoisseur.get_finger(data) is None

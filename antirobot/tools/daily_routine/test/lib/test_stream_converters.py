#!/usr/bin/env python
# -*- coding: utf-8 -*-
import logging
import pytest

from nile.api.v1 import Record

from antirobot.tools.daily_routine.lib import stream_converters

logger = logging.getLogger("test_logger")


@pytest.fixture
def sc_sorted_equal_cases():
    return [
        {
            "stream_converter": stream_converters.ParseWatchlog(),
            "in": [
                [
                    Record(
                        headerargs="something;yandexuid=12345678901234567;something",
                        counterid="1",
                        clientip="8.8.8.8",
                    ),
                ],
            ],
            "out": [
                [
                    Record(yandexuid="12345678901234567", y_counters=1),
                ],
                [
                    Record(ip="8.8.8.8", ip_counters=1),
                ],
            ],
        },
        {
            "stream_converter": stream_converters.ParseWatchlog(),
            "in": [
                [
                    Record(
                        headerargs="something;yandexuid=12345678901234567;something",
                        counterid="1",
                        clientip="8.8.8.8",
                    ),
                    Record(
                        headerargs="something;yandexuid=12345678901234567;something",
                        counterid="2",
                        clientip="8.8.8.9",
                    ),
                    Record(
                        headerargs="something;yandexuid=12345678901234568;something",
                        counterid="3",
                        clientip="8.8.8.9",
                    ),
                ],
            ],
            "out": [
                [
                    Record(yandexuid="12345678901234567", y_counters=2),
                    Record(yandexuid="12345678901234568", y_counters=1),
                ],
                [
                    Record(ip="8.8.8.8", ip_counters=1),
                    Record(ip="8.8.8.9", ip_counters=2),
                ],
            ],
        },
    ]


def test_sc_sorted_equal(sc_sorted_equal_cases):
    for case in sc_sorted_equal_cases:
        for _in, _out in zip(case["stream_converter"].test(*case["in"]), case["out"]):
            assert sorted(_in) == sorted(_out)

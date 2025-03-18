import logging
from collections import defaultdict

import pytest
import yt.wrapper as yt

import test_runner

CONTROL_EVENT = {
    ("show", "preview"): 5,
    ("show", "regular"): 92,
    ("play", "regular"): 1,
    ("end", "regular"): 1,
    ("deepwatch", "regular"): 6,
    ("autopause", "regular"): 21,
    ("pause", "regular"): 2,
    ("favourite_button:show", "regular"): 6,
    ("autoplay", "regular"): 22,
    ("click", "regular"): 11
}

CONTROL_SESSIONS = {
    "b38ce9c5d5d1d9f8349bfa3298188857": 166,
    "85f804636ce3b97b5e46ecadce747ca2": 1
}

squeeze_path = "//home/mstand/squeeze/testids/zen/rec:ab:surplus_v3_second_try:surplus_v3_v2/20200811"


def check_control_values(table, events, sessions, events_by_session):
    event_counter = defaultdict(int)
    events_by_session_counter = defaultdict(int)
    distinct_session_ids = set()

    for row in table:
        event = row.get("event")
        card_type = row.get("card_type")
        session_id = row.get("session_id")

        event_counter[(event, card_type)] += 1
        events_by_session_counter[session_id] += 1
        distinct_session_ids.add(session_id)

    for event in events:
        assert events[event] == event_counter.get(event), "Counter of {} did not pass check".format(event)

    for session in events_by_session:
        logging.debug(session)
        assert events_by_session[session] == events_by_session_counter.get(session), "Counter of session {} did not pass check".format(session)

    assert len(distinct_session_ids) == sessions, "Sessions did not pass check"


@pytest.skip("Test must be fixed")
def test_squeeze_pool_zen(yt_stuff):
    client = yt_stuff.get_yt_client()
    assert yt.exists('//home/recommender/zen/export/experiment2id', client=client)
    test_runner.run_test(
        client=client,
        services=["zen"],
        pool_path=test_runner.get_pool_path(__file__),
        path_to_row_count={
            squeeze_path: 167
        },
    )

    assert yt.exists(squeeze_path,  client=client)
    table_iterator = yt.read_table(squeeze_path, client=client)
    check_control_values(table_iterator, CONTROL_EVENT, 2, CONTROL_SESSIONS)

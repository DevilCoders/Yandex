# coding=utf-8
import datetime
import itertools
from collections import OrderedDict

import pytest

import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.test_helpers as utest

from experiment_pool import Experiment
from experiment_pool import Observation
from experiment_pool import ObservationFilters
from mstand_enums.mstand_online_enums import TableTypeEnum
from session_squeezer import squeezer, experiment_for_squeeze


def record_to_usersession(record):
    yuid = record["_yuid"]
    ts = record["_ts"]

    del record["_yuid"]
    del record["_ts"]

    value = u"\t".join(u"{}={}".format(*pair) for pair in record.items())
    result = {"key": yuid, "subkey": ts, "value": value}
    return result


def read_user_session_entries(fd):
    current_record = OrderedDict()
    for line in fd:
        line = line.strip()
        if line:
            if line.startswith("#"):
                continue

            key, value = line.split("=", 1)
            current_record[key] = value
        else:
            yield record_to_usersession(current_record)
            current_record = OrderedDict()

    if current_record:
        yield record_to_usersession(current_record)


def read_user_session(name):
    path = utest.get_source_path("session_squeezer/tests/fat/data/{}".format(name))
    for obj in read_user_session_entries(ufile.fopen_read(path, use_unicode=True)):
        obj["value"] = obj["value"].encode("utf-8")
        yield obj


def squeeze_rows(rows, testid, filters=None, service="web", use_yuids=False):
    exp = Experiment(testid=testid)
    obs_filters = ObservationFilters(filter_hash="test", filters=filters)
    obs = Observation(obs_id=None, dates=None, control=exp, filters=obs_filters)

    exp_for_squeeze = experiment_for_squeeze.ExperimentForSqueeze(exp, obs, service)
    sq = squeezer.UserSessionsSqueezer({exp_for_squeeze}, datetime.date(year=2016, month=6, day=5))
    table_types = (
        [TableTypeEnum.YUID, TableTypeEnum.SOURCE]
        if use_yuids
        else [TableTypeEnum.SOURCE]
    )
    for yuid, group in itertools.groupby(rows, key=lambda r: r["key"]):
        for exp_squeezed, action in sq.squeeze_session(yuid, group, table_types=table_types):
            assert exp_squeezed == exp_for_squeeze
            yield action


def squeeze_user_session(filename, testid, filters=None, service="web", use_yuids=False):
    user_session_rows = read_user_session(filename)
    return squeeze_rows(user_session_rows, testid, filters, service, use_yuids)


def squeeze_json(name, testid, filters=None, service="web", use_yuids=False):
    path = utest.get_source_path("session_squeezer/tests/fat/data/{}".format(name))
    rows = []
    for line in ufile.fopen_read(path, use_unicode=True):
        obj = ujson.load_from_str(line)
        obj["value"] = obj["value"].encode("utf-8")
        rows.append(obj)
    return squeeze_rows(rows, testid, filters, service, use_yuids)


# noinspection PyClassHasNoInit
class TestSqueezer:
    # noinspection PyShadowingNames
    def test_squeeze_user_sessions(self):
        request, click = squeeze_user_session("samples.us", "all")
        assert request["type"] == "request"
        assert request["query"] == request["correctedquery"] == "тест"
        assert request["userregion"] == 12345
        assert request["ts"] == 1465148167

        assert click["type"] == "click"
        assert click["url"] == "http://example.com/"
        assert click["ts"] == 1465148170

        assert request["domregion"] == click["domregion"] == "ru"
        assert request["servicetype"] == click["servicetype"] == "web"
        assert request["reqid"] == click["reqid"] == "my_awesome_reqid"
        assert request["yuid"] == click["yuid"] == "yuid_for_testing"

    def test_wrong_testid(self):
        results = list(squeeze_user_session("samples.us", "not a real testid"))
        assert not results

    def test_correct_testid(self):
        results = list(squeeze_user_session("samples_with_testids.us", "25426"))
        assert len(results) == 4
        assert results[0]["type"] == "request"
        assert results[0]["is_match"]
        assert results[1]["type"] == "click"
        assert results[1]["is_match"]
        assert results[2]["type"] == "request"
        assert results[2]["is_match"]
        assert results[3]["type"] == "click"
        assert results[3]["is_match"]

    def test_broken(self):
        results = list(squeeze_user_session("samples_broken.us", "all"))
        assert len(results) == 0

    def test_filters(self):
        results = list(squeeze_user_session("samples_with_testids.us", "25426", [("domain_filter", "ru")]))
        assert len(results) == 4
        assert results[0]["type"] == "request"
        assert results[0]["is_match"]
        assert results[1]["type"] == "click"
        assert results[1]["is_match"]

        assert results[2]["type"] == "request"
        assert not results[2]["is_match"]
        assert results[3]["type"] == "click"
        assert not results[3]["is_match"]

        results = list(squeeze_user_session("samples_with_testids.us", "25426", [("domain_filter", "kz")]))
        assert len(results) == 4
        assert not results[0]["is_match"]
        assert not results[1]["is_match"]
        assert not results[2]["is_match"]
        assert not results[3]["is_match"]

        results = list(squeeze_user_session("samples_kz.us", "25426", [("domain_filter", "kz")]))
        assert len(results) == 2
        assert results[0]["type"] == "request"
        assert results[0]["is_match"]
        assert results[1]["type"] == "click"
        assert results[1]["is_match"]

    def test_create_libra_filter(self):
        with pytest.raises(Exception):
            squeezer.libra_create_filter(ObservationFilters())

    def test_yuid_mode(self):
        results = list(squeeze_json("samples_with_yuids.json", "25426", use_yuids=True))
        yuids = set(a["yuid"] for a in results)
        assert yuids == {"yuid_for_testing"}

    def test_wrong_table_index(self):
        with pytest.raises(Exception):
            list(squeeze_json("samples_broken_index.json", "25426", use_yuids=True))

    def test_search_props(self):
        request, click = squeeze_user_session("samples.us", "all")
        assert not request["userpersonalization"]
        assert request["minrelevpredict"] is None
        assert request["maxrelevpredict"] is None

        request = list(squeeze_user_session("samples_personalization.us", "all"))[0]
        assert request["userpersonalization"]
        assert request["minrelevpredict"] == 0
        assert request["maxrelevpredict"] == 1

    def test_morda(self):
        request_ru, request_kz = squeeze_user_session("samples_morda.us", "all", service="morda")
        assert request_ru["type"] == request_kz["type"] == "request"
        assert request_ru["servicetype"] == request_kz["servicetype"] == "morda"

        assert request_ru["domregion"] == "ru"
        assert request_kz["domregion"] == "kz"

        assert request_ru["browser"] == "Chrome"
        assert request_kz["browser"] == "Firefox"

    def test_query_region(self):
        request, req_123aaa, req_aaaa, req_long = squeeze_user_session("samples_query_region.us", "all")
        assert request["queryregion"] == 1234
        assert req_123aaa["queryregion"] == 123
        assert "queryregion" not in req_aaaa
        assert "queryregion" not in req_long

    def test_search_engine_clicks(self):
        request, click_google, click_bing, click_mail = squeeze_user_session("samples_search_engine_clicks.us", "all")
        assert click_google["target"] == "google_search"
        assert click_bing["target"] == "bing_search"
        assert click_mail["target"] == "mail_search"

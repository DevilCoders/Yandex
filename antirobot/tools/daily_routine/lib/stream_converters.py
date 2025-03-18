# -*- coding: utf-8 -*-
from __future__ import division
import re

# from nile.api.v1 import filters as nf
from nile.api.v1 import extractors as ne
from nile.api.v1 import aggregators as na
from nile.api.v1 import Record
from nile.api.v1 import with_hints
from qb2.api.v1 import typing as st
import nile_tools
import event_log_reader

MAX_SESSION_LENGTH = 60 * 60 * 10**6
EVENT_CODING = {
    "TCaptchaRedirect": "R",
    "TCaptchaShow": "S",
    "TCaptchaImageShow": "I",
    "TCaptchaCheck": "C",
    "TAntirobotFactors": "F",
    "TRequestData": "D",
}


class Combo(nile_tools.StreamConverter):
    def __call__(self,
                 event_stream,
                 daemon_stream,
                 watchlog_stream,
                 fraud_uids_stream,
                 frauds_stream,
                 clean_stream):
        parsed_event_stream, = ParseEventLog()(event_stream)
        factors_stream, = FormFactors()(parsed_event_stream, daemon_stream)
        watchlog_yandexuid_stream, watchlog_ip_stream = ParseWatchlog()(watchlog_stream)
        us_stream, = ParseUsersessions()(fraud_uids_stream, frauds_stream, clean_stream)

        unite_stream, = Unite()(factors_stream, watchlog_ip_stream, watchlog_yandexuid_stream, us_stream)

        return unite_stream, parsed_event_stream, factors_stream, watchlog_yandexuid_stream, watchlog_ip_stream, us_stream


class Unite(nile_tools.StreamConverter):
    """
    Объединяет табличку с факторами с остальными не менее важными табличками
    """
    def __call__(self, factors_stream, watchlog_ip_stream, watchlog_yandexuid_stream, us_stream):
        stream = factors_stream.join(watchlog_ip_stream, by="ip", type="left")
        stream = stream.join(watchlog_yandexuid_stream, by="yandexuid", type="left")
        stream = stream.project(ne.all(), breq_id=ne.custom(lambda x: x[:x.index("-") + 6] if "-" in x else None, "req_id"))
        stream = stream.join(us_stream, by="breq_id", type="left")
        return stream.sort("timestamp"),


class FormFactors(nile_tools.StreamConverter):
    """
    Формирует табличку с факторами и различными флагами, приклеивает к ней демонлог
    """

    @staticmethod
    def process_chain(chain):
        event_factors = {
            "redirects": 0,
            "shows": 0,
            "image_shows": 0,
            "checks": 0,
            "tokens_cnt": 0,
            "chain_len": len(chain),
            "chain_sec": (int(chain[-1].timestamp) - int(chain[0].timestamp)) // 10**6,
            "tokens_count": 0,
            "failures": 0,
            "timeouted": False,
            "succeeded": False,
        }

        data = ""
        yandexuid = ""
        tokens = set()

        for record in chain:
            if record.event_type == "TRequestData" and record.reqid == chain[0].reqid:
                data = record.rest.get("data", "")
            if record.get("token"):
                tokens.add(record.token)
            if record.event_type == "TCaptchaRedirect":
                event_factors["redirects"] += 1
            if record.event_type == "TCaptchaShow":
                event_factors["shows"] += 1
            if record.event_type == "TCaptchaImageShow":
                event_factors["image_shows"] += 1
            if record.event_type == "TCaptchaCheck":
                event_factors["checks"] += 1
                if record.rest["captcha_connect_error"]:
                    event_factors["timeouted"] = True
                    event_factors["succeeded"] = True
                elif record.rest["success"]:
                    event_factors["succeeded"] = True
                else:
                    event_factors["failures"] += 1
            yandexuid = yandexuid or record.yandex_uid
        event_factors["tokens_cnt"] = len(tokens)
        return Record(
            ident_type=chain[0].uid,
            factors=chain[0].rest["factors"],
            factors_version=chain[0].rest["factors_version"],
            req_id=chain[0].reqid,
            timestamp=chain[0].timestamp,
            is_robot=chain[0].rest["is_robot"],
            ip=chain[0].addr,
            event_factors=event_factors,
            data=data,
            chain="".join([EVENT_CODING.get(item.event_type, "X") for item in chain]),
            yandexuid=yandexuid,
        )

    def reducer(self, groups):
        for key, records in groups:
            chain = []
            for record in records:
                if record.event_type == "TAntirobotFactors":
                    if chain:
                        yield self.process_chain(chain)
                    chain = [record]
                elif record.event_type == "TCaptchaCheck" and record.rest["success"] and chain:
                    chain.append(record)
                    yield self.process_chain(chain)
                    chain = []
                elif chain:
                    chain.append(record)
            if chain and chain[0].event_type == "TAntirobotFactors":
                yield self.process_chain(chain)

    def __call__(self, event_stream, daemon_stream):
        """
        tokens = event_stream \
            .filter(sf.nonzero("token")) \
            .groupby("token") \
            .aggregate(uids_on_token=na.count_distinct("uid"),
                       ips_on_token=na.count_distinct("addr"),
                       records_on_token=na.count())
        """
        chains = event_stream \
            .groupby("uid") \
            .sort("timestamp") \
            .reduce(self.reducer, memory_limit=8000)

        daemon_projected = daemon_stream.project("req_id",
                                                 "req_type",
                                                 "verdict",
                                                 "matrixnet")

        joined = chains.join(daemon_projected, by="req_id", type="left", assume_unique=True) \
            .project(ne.all("matrixnet"), mxnet=ne.custom(lambda x: float(x) if x else None, "matrixnet")) \
            .sort("timestamp")

        return joined,


class ParseEventLog(nile_tools.StreamConverter):
    """
    Переводит в читабельный вид event log
    """
    @staticmethod
    def mapper(records):
        for record in records:
            yield Record(iso_eventtime=record.iso_eventtime, **event_log_reader.parse_event(record.event))

    def __call__(self, eventlog, **options):
        return eventlog.map(self.mapper).sort("timestamp"),


class ParseWatchlog(nile_tools.StreamConverter):
    """
    Считает, сколько счетчиков за день было замечено на каждом яндексуиде и каждом айпишнике.

    """

    @staticmethod
    def cut_yuid(header):
        if not header:
            return
        yuid_position = header.find("yandexuid")
        if yuid_position == -1:
            return
        start_position = yuid_position + 10
        end_position = header.find(";", start_position)
        if end_position < start_position:
            return
        yuid = header[start_position:end_position]
        if not (17 <= len(yuid) <= 19) \
                or not yuid.isdigit() \
                or yuid[0] == "0":
            return
        return yuid

    def mapper_yandexuid(self, records):
        for record in records:
            yandexuid = self.cut_yuid(record.headerargs)
            if yandexuid:
                yield Record(yandexuid=yandexuid, counterid=record.counterid)

    def __call__(self, watchlog, **options):
        yandexuids = watchlog \
            .map(self.mapper_yandexuid) \
            .groupby("yandexuid") \
            .aggregate(y_counters=na.count_distinct("counterid")) \
            .sort("yandexuid")

        ips = watchlog \
            .groupby("clientip") \
            .aggregate(ip_counters=na.count_distinct("counterid")) \
            .project("ip_counters", ip="clientip") \
            .sort("ip")
        return yandexuids, ips


class ParseUsersessions(nile_tools.StreamConverter):
    """
    Парсит юзерсессии, выдает "роботность" запроса.
    Вместо req_id использует breq_id — число до дефиса в req_id плюс еще пять цифр от числа после.
    """

    def __init__(self):
        arules = {
            133: 100,  # 133: incorrect pagination visits. may ban for sure
            139: 50,  # 139: >50% network to images, unique request, ppl with unique > 70%
            141: 100,
            142: 75,
            153: 50,
            157: 0,
            167: 25,
            168: 50,
            171: 50,
            179: 50,
            185: 75,
            186: 75,
            190: 25,
            193: 25,
            194: 50,
            195: 50,
            230: 75,
            272: 25,  # seo
            279: 50,
            298: 50,
        }

        srules = {
            1: 100,  # bad user agent (regular)
            2: 100,  # 100 bad yandexuids, not as per documentation
            10: 50,  # bad cgi, like (link, numdoc, ...)
            11: 25,  # wide lang requests. can be ppl. url, inurl, date, head, title
            12: 25,  # same but to images.yandex.ru
        }

        self.rules = {}
        self.rules.update({"ARules=" + str(rule): arules[rule] for rule in arules})
        self.rules.update({"SRules=" + str(rule): srules[rule] for rule in srules})

        self.mapper_fraud_uids_schema = with_hints(output_schema={"robotness_us": st.Float,
                                                                  "yandexuid": st.String,
                                                                  "robot_rules_us": st.List[st.String]})
        self.mapper_schema = with_hints(output_schema={"breq_id": st.String,
                                                       "yandexuid": st.String,
                                                       "is_fraud_us": st.Bool})

    def get_robot_rules(self, value):
        return [rule for rule in self.rules if rule in value]

    def get_robotness(self, value):
        return sum(self.rules[rule] for rule in self.rules if rule in value) / 100

    def mapper_fraud_uids(self, records):
        for record in records:
            if record.key and record.key[0] == "y":
                yield Record(
                    yandexuid=record.key[1:],
                    robotness_us=self.get_robotness(record.value),
                    robot_rules_us=self.get_robot_rules(record.value),
                )

    @staticmethod
    def get_breq_id_by_value(value):
        a = re.search("reqid=(\d{16}-\d{5})", value)
        return a.group(1) if a else None

    def mapper(self, records):
        for record in records:
            if record.key and record.key[0] == "y":
                breq_id = self.get_breq_id_by_value(record.value)
                if breq_id:
                    yield Record(
                        yandexuid=record.key[1:],
                        breq_id=breq_id,
                        is_fraud_us=record.is_fraud_us,
                    )

    def __call__(self, fraud_uids, frauds, clean, **options):
        clean_proj = clean.project(ne.all(), is_fraud_us=ne.const(False))
        frauds_proj = frauds.project(ne.all(), is_fraud_us=ne.const(True))
        fraud_uids_proj = fraud_uids.map(self.mapper_fraud_uids_schema(self.mapper_fraud_uids))
        return clean_proj \
            .concat(frauds_proj) \
            .map(self.mapper_schema(self.mapper), intensity="data") \
            .join(fraud_uids_proj, by="yandexuid", type="left") \
            .unique("breq_id") \
            .sort("breq_id"),

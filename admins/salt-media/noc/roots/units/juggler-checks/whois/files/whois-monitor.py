#!/usr/bin/pynoc

import argparse
import textwrap
import io
import sys
import requests
import logging as log
from datetime import datetime
from collections import defaultdict
from dataclasses import dataclass

log.basicConfig(
    format="%(asctime)s %(levelname)5s %(funcName)s:%(lineno)-4s %(message)s",
    level=log.INFO,
    stream=sys.stderr,
)


@dataclass
class Alert:
    reg: str
    age: float
    msg: str


class AlertError(Alert):
    pass


class AlertStale(Alert):
    pass


AGE_IN_SECONDS_IS_OK = 3600 * 24
CRITICAL_REGS = ["RIPE", "AFRINIC", "ARIN", "APNIC", "RADB"]
TIME_FORMAT = "%Y-%m-%d %H:%M:%S"


def parse_status(status_text):
    category = ""
    status_for = ""
    status = defaultdict(dict)
    for line in status_text.readlines():
        line = line.strip()

        if line.startswith("Status for"):
            status_for = line.split(" ")[2]
        if status_for == "":
            continue

        if line.startswith("Local information"):
            category = "local"
            continue
        if category != "local":
            continue

        key, _, data = line.partition(": ")
        key = key.replace(" ", "_").lower()

        if key.startswith("last_update") or key.startswith("last_import_error"):
            if data == "None":
                continue
            date = data.split(".")[0]
            status[status_for][key] = datetime.strptime(date, TIME_FORMAT)
    return status


def monitor(status, now) -> list[Alert]:
    alerts: list[Alert] = []
    for reg, val in status.items():
        update = val["last_update"]
        err = val.get("last_import_error_occurred_at")
        log.debug("%s: update=%r, err=%r", reg, update, err)
        if err:
            delta = err.timestamp() - update.timestamp()

            if delta > 3600 * 24:
                err_period = pretty_print_time_delta(delta)
                log.debug("update err time=%s", err_period)
                alerts.append(AlertError(reg, delta, f"update err {err_period}"))
                continue

        delta = now.timestamp() - update.timestamp()
        if delta > AGE_IN_SECONDS_IS_OK:
            age = pretty_print_time_delta(delta)
            log.debug("last update %s ago", age)
            alerts.append(AlertStale(reg, delta, f"last update {age} ago"))
    return alerts


def pretty_print_time_delta(seconds):
    days, remainder = divmod(int(seconds), 3600 * 24)
    hours, remainder = divmod(remainder, 3600)
    minutes, seconds = divmod(remainder, 60)
    days_str, hours_str, minutes_str, seconds_str = "", "", "", ""
    if days > 0:
        days_str = "{}d".format(days)
    if hours > 0:
        hours_str = "{}h".format(hours)  # for python2.7/3.5 compatibility
    if minutes > 0:
        minutes_str = "{}m".format(minutes)
    if (hours == 0 and minutes == 0) or seconds > 0:
        seconds_str = "{}s".format(seconds)
    return "{}{}{}{}".format(days_str, hours_str, minutes_str, seconds_str)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-l", "--loglevel", help="logging level")
    args = parser.parse_args()
    if args.loglevel:
        log.getLogger().setLevel(args.loglevel.upper())
    resp = requests.get("http://localhost:8080/v1/status")
    resp.raise_for_status()
    status = parse_status(io.StringIO(resp.text))
    alerts: list[Alert] = monitor(status, datetime.now())

    important_alerts: dict[str, Alert] = {alert.reg: alert for alert in alerts if alert.reg in CRITICAL_REGS}
    for reg in CRITICAL_REGS:
        level = "OK"
        alert = important_alerts.get(reg)
        if alert:
            if isinstance(alert, AlertError):
                level = "CRIT"
            else:
                level = "WARN"
                if alert.age > AGE_IN_SECONDS_IS_OK * 2:
                    level = "CRIT"
        print(f"PASSIVE-CHECK:whois-{reg};{level};{'OK' if not alert else alert.msg}")


# TESTS
_test_data = textwrap.dedent(
    """
    IRRD version 4.2.0
    Listening on ::0 port 43


    ------------------------------------------------------------------------------
     source          total obj     rt obj    aut-num obj    serial    last export
    ------------------------------------------------------------------------------
     AFRINIC            366460      99630           2185      None
     ALTDB               29704      21356           1483      None
     APNIC             2429985     631191          19490      None
     ARIN                94488      65168           2619      None
     ARIN-NONAUTH        34334      29291            932      None
     BBOI                 1428        925             65      None
     BELL                30171      29885            106      None
     CANARIE              1858       1424            174      None
     HOST                    3          0              1      None
     JPIRR               14789      12631            442      None
     LEVEL3             120374      90139            300      None
     NESTEGG                 8          4              2      None
     NTTCOM             463196     454225            545      None
     OPENFACE               26         17              1      None
     PANIX                  42         40              1      None
     RADB              1599586    1401759           9162      None
     REACH               20273      18168              2      None
     RGNET                  49         28              6      None
     RIPE              6588600     384221          37713      None
     RIPE-NONAUTH        57457      53929           2133      None
     RPKI               363304     291035              0      None
     TC                  31146      13648           3864      None
     TOTAL            12247281    3598714          81226


    Status for AFRINIC
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 1230262
        Synchronised NRTM serials: No
        Last update: 2022-07-26 03:40:19.817215+03:00
        Local journal kept: No
        Last import error occurred at: 2022-07-26 03:40:19.815485+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for ALTDB
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 95315
        Synchronised NRTM serials: No
        Last update: 2022-07-26 01:42:01.411260+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: whois.altdb.net port 43
        Mirrorable: Yes
        Oldest journal serial number: 87143
        Newest journal serial number: 95315
        Last export at serial number: 95315


    Status for APNIC
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 10351253
        Synchronised NRTM serials: No
        Last update: 2022-07-25 18:33:55.847031+03:00
        Local journal kept: No
        Last import error occurred at: 2022-07-25 18:33:55.846197+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for ARIN
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 360274
        Synchronised NRTM serials: No
        Last update: 2022-07-26 13:26:56.554594+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: rr.arin.net port 43
        Mirrorable: Yes
        Oldest journal serial number: 0
        Newest journal serial number: 360274
        Last export at serial number: None


    Status for ARIN-NONAUTH
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 126601
        Synchronised NRTM serials: No
        Last update: 2022-04-04 13:58:15.168721+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for BBOI
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 18919
        Synchronised NRTM serials: No
        Last update: 2022-07-08 19:47:24.535723+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for BELL
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 249677
        Synchronised NRTM serials: No
        Last update: 2022-07-15 02:15:51.915915+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for CANARIE
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 4470
        Synchronised NRTM serials: No
        Last update: 2022-07-08 22:47:45.913583+03:00
        Local journal kept: No
        Last import error occurred at: 2022-07-08 22:47:45.912605+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for HOST
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 782
        Synchronised NRTM serials: No
        Last update: 2022-03-25 16:19:35.624852+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for JPIRR
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 226920
        Synchronised NRTM serials: No
        Last update: 2022-07-26 08:23:32.040325+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for LEVEL3
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 835734
        Synchronised NRTM serials: No
        Last update: 2022-06-29 00:27:07.662117+03:00
        Local journal kept: No
        Last import error occurred at: 2022-07-26 15:18:53.369336+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: rr.level3.net port 43
        Remote status query unsupported or query failed


    Status for NESTEGG
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 43
        Synchronised NRTM serials: No
        Last update: 2022-03-25 16:19:36.042423+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for NTTCOM
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 1295379
        Synchronised NRTM serials: No
        Last update: 2022-06-29 12:53:14.277235+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: rr.ntt.net port 43
        Mirrorable: Yes
        Oldest journal serial number: 0
        Newest journal serial number: 1293190
        Last export at serial number: 1297792


    Status for OPENFACE
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 59
        Synchronised NRTM serials: No
        Last update: 2022-03-25 16:19:49.512224+03:00
        Local journal kept: No
        Last import error occurred at: 2022-03-25 16:19:49.510607+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for PANIX
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 62
        Synchronised NRTM serials: No
        Last update: 2022-03-25 16:19:52.409279+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for RADB
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 13694429
        Synchronised NRTM serials: No
        Last update: 2022-07-26 13:42:13.529351+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: whois.radb.net port 43
        Mirrorable: Yes
        Oldest journal serial number: 13610959
        Newest journal serial number: 13694486
        Last export at serial number: None


    Status for REACH
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 870578
        Synchronised NRTM serials: No
        Last update: 2022-07-26 00:48:15.522475+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for RGNET
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 466
        Synchronised NRTM serials: No
        Last update: 2022-03-25 16:20:04.984929+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for RIPE
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 53912143
        Synchronised NRTM serials: No
        Last update: 2022-07-24 13:41:43.119982+03:00
        Local journal kept: No
        Last import error occurred at: 2022-07-26 12:40:09.969228+03:00
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: whois.ripe.net port 4444
        Remote status query unsupported or query failed


    Status for RIPE-NONAUTH
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 53912143
        Synchronised NRTM serials: No
        Last update: 2022-07-26 13:41:42.860829+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        NRTM host: whois.ripe.net port 4444
        Remote status query unsupported or query failed


    Status for RPKI
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: None
        Synchronised NRTM serials: No
        Last update: 2022-07-26 13:25:59.907276+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.


    Status for TC
    -------------------
    Local information:
        Authoritative: No
        Object class filter: None
        Oldest serial seen: None
        Newest serial seen: None
        Oldest local journal serial number: None
        Newest local journal serial number: None
        Last export at serial number: None
        Newest serial number mirrored: 154245
        Synchronised NRTM serials: No
        Last update: 2022-07-26 02:33:49.691170+03:00
        Local journal kept: No
        Last import error occurred at: None
        RPKI validation enabled: Yes
        Scope filter enabled: No

    Remote information:
        No NRTM host configured.
    """
)


def test_parse_status():
    expected_output = textwrap.dedent(
        """
           AFRINIC, 2022-07-26T03:40:19 (err: 2022-07-26T03:40:19)
           ALTDB, 2022-07-26T01:42:01 (err: None)
           APNIC, 2022-07-25T18:33:55 (err: 2022-07-25T18:33:55)
           ARIN, 2022-07-26T13:26:56 (err: None)
           ARIN-NONAUTH, 2022-04-04T13:58:15 (err: None)
           BBOI, 2022-07-08T19:47:24 (err: None)
           BELL, 2022-07-15T02:15:51 (err: None)
           CANARIE, 2022-07-08T22:47:45 (err: 2022-07-08T22:47:45)
           HOST, 2022-03-25T16:19:35 (err: None)
           JPIRR, 2022-07-26T08:23:32 (err: None)
           LEVEL3, 2022-06-29T00:27:07 (err: 2022-07-26T15:18:53)
           NESTEGG, 2022-03-25T16:19:36 (err: None)
           NTTCOM, 2022-06-29T12:53:14 (err: None)
           OPENFACE, 2022-03-25T16:19:49 (err: 2022-03-25T16:19:49)
           PANIX, 2022-03-25T16:19:52 (err: None)
           RADB, 2022-07-26T13:42:13 (err: None)
           REACH, 2022-07-26T00:48:15 (err: None)
           RGNET, 2022-03-25T16:20:04 (err: None)
           RIPE, 2022-07-24T13:41:43 (err: 2022-07-26T12:40:09)
           RIPE-NONAUTH, 2022-07-26T13:41:42 (err: None)
           RPKI, 2022-07-26T13:25:59 (err: None)
           TC, 2022-07-26T02:33:49 (err: None)
    """
    ).lstrip()

    status = parse_status(io.StringIO(_test_data))
    out = io.StringIO()
    for reg in status:
        update = status[reg]['last_update'].strftime("%FT%T")
        error_tm = status[reg].get('last_import_error_occurred_at')
        error = None if not error_tm else error_tm.strftime("%FT%T")
        out.write(f"{reg}, {update} (err: {error})\n")
    assert expected_output == out.getvalue()


def test_alerts():
    expected_alerts = [
        AlertStale(reg='ARIN-NONAUTH', age=9762238.0, msg='last update 112d23h43m58s ago'),
        AlertStale(reg='BBOI', age=1533289.0, msg='last update 17d17h54m49s ago'),
        AlertStale(reg='BELL', age=991582.0, msg='last update 11d11h26m22s ago'),
        AlertStale(reg='CANARIE', age=1522468.0, msg='last update 17d14h54m28s ago'),
        AlertStale(reg='HOST', age=10617758.0, msg='last update 122d21h22m38s ago'),
        AlertError(reg='LEVEL3', age=2386306.0, msg='update err 27d14h51m46s'),
        AlertStale(reg='NESTEGG', age=10617757.0, msg='last update 122d21h22m37s ago'),
        AlertStale(reg='NTTCOM', age=2335739.0, msg='last update 27d48m59s ago'),
        AlertStale(reg='OPENFACE', age=10617744.0, msg='last update 122d21h22m24s ago'),
        AlertStale(reg='PANIX', age=10617741.0, msg='last update 122d21h22m21s ago'),
        AlertStale(reg='RGNET', age=10617729.0, msg='last update 122d21h22m9s ago'),
        AlertError(reg='RIPE', age=169106.0, msg='update err 1d22h58m26s'),
    ]

    status = parse_status(io.StringIO(_test_data))
    alerts = monitor(status, datetime.strptime("2022-07-26 13:42:13", TIME_FORMAT))
    print(alerts)
    assert expected_alerts == alerts

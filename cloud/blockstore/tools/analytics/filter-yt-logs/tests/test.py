import importlib
import logging
import os
import sys
import yt.wrapper as yt


lib = importlib.import_module("cloud.blockstore.tools.analytics.filter-yt-logs.lib")

logger = logging.getLogger('test')
handler = logging.StreamHandler(sys.stderr)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)


def test_filter_nbs_logs():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_nbs_logs_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": "2019-10-11T08:23:29.082126Z :BLOCKSTORE_SERVICE INFO: Unmounting volume: "
                       "\"fhmffbfd3c1shvt40sts\" (client: \"27fedfb4-645cf04f-bf8aaa33-f228dbe6\")",
            "SYSLOG_IDENTIFIER": "NBS_SERVER",
            "iso_eventtime": "2019-10-11 11:47:01"
        }
    ])

    lib.filter_nbs_logs(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "2019-10-11T08:23:29.082126Z :BLOCKSTORE_SERVICE INFO: Unmounting volume: "
                       "\"fhmffbfd3c1shvt40sts\" (client: \"27fedfb4-645cf04f-bf8aaa33-f228dbe6\")",
            "component": "BLOCKSTORE_SERVICE",
            "priority": "INFO",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2019-10-11 11:47:01",
            "syslog_identifier": "NBS_SERVER"
        }
    ]


def test_filter_nbs_ua_logs():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_nbs_ua_logs_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": "Unmounting volume: "
                       "\"fhmffbfd3c1shvt40sts\" (client: \"27fedfb4-645cf04f-bf8aaa33-f228dbe6\")",
            "IDENTIFIER": "NBS_SERVER",
            "TIMESTAMP": "1654173410598245",
            "COMPONENT": "BLOCKSTORE_SERVICE",
            "PRIORITY": "INFO"
        }
    ])

    lib.filter_nbs_logs(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger,
        True
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "Unmounting volume: "
                       "\"fhmffbfd3c1shvt40sts\" (client: \"27fedfb4-645cf04f-bf8aaa33-f228dbe6\")",
            "component": "BLOCKSTORE_SERVICE",
            "priority": "INFO",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2022-06-02T12:36:50.598245",
            "syslog_identifier": "NBS_SERVER"
        }
    ]


def test_filter_nbs_traces():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_nbs_traces_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": "2019-10-11T08:23:30.409842Z :BLOCKSTORE_TRACE WARN: [\"RequestStarted\",0,\"WriteBlocks\",\"1\""
                       ",\"42998446277982480\",\"fhmlp8t1e8kn5198pt8a\"]",
            "SYSLOG_IDENTIFIER": "NBS_SERVER",
            "iso_eventtime": "2019-10-11 11:47:01"
        }
    ])

    lib.filter_nbs_traces(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "2019-10-11T08:23:30.409842Z :BLOCKSTORE_TRACE WARN: [\"RequestStarted\",0,\"WriteBlocks\",\"1\""
                       ",\"42998446277982480\",\"fhmlp8t1e8kn5198pt8a\"]",
            "component": "BLOCKSTORE_TRACE",
            "priority": "WARN",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2019-10-11 11:47:01",
            "syslog_identifier": "NBS_SERVER"
        }
    ]


def test_filter_nbs_ua_traces():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_nbs_ua_traces_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": "[\"RequestStarted\",0,\"WriteBlocks\",\"1\""
                       ",\"42998446277982480\",\"fhmlp8t1e8kn5198pt8a\"]",
            "IDENTIFIER": "NBS_SERVER",
            "TIMESTAMP": "1654173410598245",
            "COMPONENT": "BLOCKSTORE_TRACE",
            "PRIORITY": "WARN"
        }
    ])

    lib.filter_nbs_traces(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger,
        True
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "[\"RequestStarted\",0,\"WriteBlocks\",\"1\""
                       ",\"42998446277982480\",\"fhmlp8t1e8kn5198pt8a\"]",
            "component": "BLOCKSTORE_TRACE",
            "priority": "WARN",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2022-06-02T12:36:50.598245",
            "syslog_identifier": "NBS_SERVER"
        }
    ]


def test_filter_qemu_logs():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_qemu_logs_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": """D: [i=epds5leehmefbds9m2bo] [o=epdnde9rorb9r2m8cbam] QEMU output:
[libprotobuf ERROR /place/sandbox-data/tasks/5/7/516610475/arcadia/contrib/libs/protobuf/text_format.cc:288]
2019-10-08-09-18-12 :BLOCKSTORE_PLUGIN INFO: cloud/vm/blockstore/bootstrap.cpp:183: NBS plugin version: stable-19-4-41
""",
            "UNIT": "yc-compute-node.service",
            "iso_eventtime": "2019-10-08 09-18-12"
        }
    ])

    lib.filter_qemu_logs(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "2019-10-08-09-18-12 :BLOCKSTORE_PLUGIN INFO: cloud/vm/blockstore/bootstrap.cpp:183: "
                       "NBS plugin version: stable-19-4-41",
            "component": "BLOCKSTORE_PLUGIN",
            "priority": "INFO",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2019-10-08 09-18-12",
            'syslog_identifier': None
        }
    ]


def test_filter_qemu_ua_logs():
    yt_proxy = os.environ["YT_PROXY"]
    yt.config["proxy"]["url"] = yt_proxy

    table_prefix = "//tmp/test_filter_qemu_ua_logs_{}"
    src_table = table_prefix.format('src')
    dst_table = table_prefix.format('dst')
    yt.create("table", src_table)

    yt.write_table(src_table, [
        {
            "HOSTNAME": "myt1-s7-16.cloud.yandex.net",
            "MESSAGE": "cloud/vm/blockstore/bootstrap.cpp:183: NBS plugin version: stable-19-4-41",
            "IDENTIFIER": "NBS_CLIENT",
            "TIMESTAMP": "1654173410598245",
            "COMPONENT": "BLOCKSTORE_PLUGIN",
            "PRIORITY": "INFO",
        }
    ])

    lib.filter_qemu_logs(
        src_table,
        dst_table,
        yt_proxy,
        None,
        None,
        logger,
        True
    )

    assert [_ for _ in yt.read_table(dst_table, format="json")] == [
        {
            "message": "cloud/vm/blockstore/bootstrap.cpp:183: NBS plugin version: stable-19-4-41",
            "component": "BLOCKSTORE_PLUGIN",
            "priority": "INFO",
            "host": "myt1-s7-16.cloud.yandex.net",
            "timestamp": "2022-06-02T12:36:50.598245",
            'syslog_identifier': "NBS_CLIENT"
        }
    ]

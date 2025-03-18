# -*- coding: utf-8 -*-
from library.python.monitoring.solo.objects.solomon.sensor import Sensor


def lb_sensor(**kwargs):
    predefined_labels = dict(project="kikimr")
    for key, value in predefined_labels.items():
        if key not in kwargs:
            kwargs[key] = value
    return Sensor(**kwargs)


def lb_quota_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="quoter_service",
        resource="write-quota/{}".format(topic),
        quoter="/Root/PersQueue/System/Quoters/{}".format(account),
        sensor="QuotaConsumed|Limit",
        host="{{host}}"
    )


def lb_quota_consumed_sensor(cluster, account, topic=None):
    return lb_sensor(
        cluster=cluster,
        host="Iva|Man|Myt|Sas|Vla" if cluster != "lbkx" else "cluster",
        service="quoter_service",
        resource="write-quota/{}".format(topic) if topic is not None else "write-quota",
        quoter="/Root/PersQueue/System/Quoters/{}".format(account),
        sensor="QuotaConsumed",
    )


def lb_quota_limit_sensor(cluster, account, topic=None):
    return lb_sensor(
        cluster=cluster,
        host="Iva|Man|Myt|Sas|Vla" if cluster != "lbkx" else "cluster",
        service="quoter_service",
        resource="write-quota/{}".format(topic) if topic is not None else "write-quota",
        quoter="/Root/PersQueue/System/Quoters/{}".format(account),
        sensor="Limit",
    )


def lb_bytes_written_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqproxy_writeSession",
        Account=account,
        OriginDC="{{host}}",
        host="{{host}}",
        sensor="BytesWritten*",
        TopicPath="{}/{}".format(account, topic),
        Topic="{}--{}".format(account, topic),
        Producer="!total"
    )


def lb_write_lag_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqproxy_writeTimeLag",
        Account=account,
        TopicPath="{}/{}".format(account, topic),
        Topic="{}--{}".format(account, topic),
        Producer="!total",
        OriginDC="cluster",
        host="cluster",
        sensor="TimeLagsOriginal",
        Interval="*"
    )


def lb_bytes_written_original_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqproxy_writeSession",
        Account=account,
        OriginDC="cluster",
        host="cluster",
        sensor="BytesWrittenOriginal",
        TopicPath="{}/{}".format(account, topic),
        Topic="{}--{}".format(account, topic),
    )


def lb_messages_written_original_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqproxy_writeSession",
        Account=account,
        OriginDC="cluster",
        host="cluster",
        sensor="MessagesWrittenOriginal",
        TopicPath="{}/{}".format(account, topic),
        Topic="{}--{}".format(account, topic),
    )


def lb_source_id_max_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqtabletAggregatedCounters",
        sensor="SourceIdMaxCount",
        host="cluster",
        OriginDC="cluster",
        Account=account,
        TopicPath="{}/{}".format(account, topic),
    )


def lb_partition_quota_usage_sensor(cluster, account, topic):
    return lb_sensor(
        cluster=cluster,
        service="pqtabletAggregatedCounters",
        sensor="PartitionMaxWriteQuotaUsage",
        host="Iva|Man|Myt|Sas|Vla" if cluster != "lbkx" else "dc-unknown",
        OriginDC="Iva|Man|Myt|Sas|Vla"if cluster != "lbkx" else "Kafka-bs",
        Account=account,
        TopicPath="{}/{}".format(account, topic),
    )

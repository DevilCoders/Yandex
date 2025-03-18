# -*- coding: utf-8 -*-
import datetime

from library.python.monitoring.solo.helpers.logbroker.sensors import lb_quota_consumed_sensor, lb_quota_limit_sensor, lb_bytes_written_original_sensor, \
    lb_messages_written_original_sensor, lb_source_id_max_sensor, lb_partition_quota_usage_sensor
from library.python.monitoring.solo.objects.solomon.v2 import Alert, MultiAlert, Type, Expression

# cluster is one of:
# - lbk (production logbroker)
# - lbkx (cross-dc logbroker)


def lb_topic_quota_alert(solomon_project, cluster, account, topic, threshold=0.9):
    kwargs = dict(
        id="{}_topic_quota_{}_{}".format(cluster, account, topic),
        project_id=solomon_project.id,
        name="Logbroker {} quota for {}/{} topic".format(cluster, account, topic),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#QuotaConsumed",
            "usage_percentage": "{{expression.percentage_str}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let percentage = avg({used}) / avg({limit});
                    let percentage_str = to_fixed(percentage, 2);
                    alarm_if(percentage > {threshold});
                """.format(
                    used=lb_quota_consumed_sensor(cluster, account, topic),
                    limit=lb_quota_limit_sensor(cluster, account, topic),
                    threshold=threshold
                )
            )),
        window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds()),
    )
    if cluster == "lbk":
        return MultiAlert(
            group_by_labels={
                "host"
            },
            **kwargs
        )
    elif cluster == "lbkx":
        return Alert(
            **kwargs
        )


def lb_account_quota_alert(solomon_project, cluster, account, threshold=0.9):
    kwargs = dict(
        id="{}_account_quota_{}".format(cluster, account),
        project_id=solomon_project.id,
        name="Logbroker {} quota for account {}".format(cluster, account),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#QuotaConsumed",
            "usage_percentage": "{{expression.percentage_str}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let percentage = avg({used}) / avg({limit});
                    let percentage_str = to_fixed(percentage, 2);
                    alarm_if(percentage > {threshold});
                """.format(
                    used=lb_quota_consumed_sensor(cluster, account),
                    limit=lb_quota_limit_sensor(cluster, account),
                    threshold=threshold
                )
            )),
        window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds())
    )
    if cluster == "lbk":
        return MultiAlert(
            group_by_labels={
                "host"
            },
            **kwargs
        )
    elif cluster == "lbkx":
        return Alert(
            **kwargs
        )


def lb_bytes_written_original_alert(solomon_project, cluster, account, topic, threshold=0):
    alert = Alert(
        id="{}_bw_{}_{}".format(cluster, account, topic),
        project_id=solomon_project.id,
        name="Logbroker {} Bytes Written Original for {}/{}".format(cluster, account, topic),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#BytesWrittenOriginal",
            "bytes_written": "{{expression.bytes_written}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let bytes_written = avg({});
                    alarm_if(bytes_written <= {});
                """.format(lb_bytes_written_original_sensor(cluster, account, topic), threshold)
            )),
        window_secs=int(datetime.timedelta(minutes=30).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds()),
    )
    return alert


def lb_messages_written_original_alert(solomon_project, cluster, account, topic, threshold=0):
    alert = Alert(
        id="{}_mw_{}_{}".format(cluster, account, topic),
        project_id=solomon_project.id,
        name="Logbroker {} Messages Written Original for {}/{}".format(cluster, account, topic),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#MessagesWrittenOriginal",
            "messages_written": "{{expression.messages_written}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let messages_written = avg({});
                    alarm_if(messages_written <= {});
                """.format(lb_messages_written_original_sensor(cluster, account, topic), threshold)
            )),
        window_secs=int(datetime.timedelta(minutes=30).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds()),
    )
    return alert


def lb_source_id_max_alert(solomon_project, account, cluster, topic, threshold=10000):
    alert = Alert(
        id="{}_sourceid_{}_{}".format(cluster, account, topic),
        project_id=solomon_project.id,
        name="Logbroker {} SourceId Max Count for {}/{}".format(cluster, account, topic),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#SourceIdMaxCount",
            "source_id_max": "{{expression.source_id_max}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let source_id_max = avg({});
                    alarm_if(source_id_max > {});
                """.format(lb_source_id_max_sensor(cluster, account, topic), threshold)
            )),
        window_secs=int(datetime.timedelta(minutes=10).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds()),
    )
    return alert


def lb_topic_partition_quota_usage_alert(solomon_project, cluster, account, topic, threshold=1000000):
    kwargs = dict(
        id="{}_partition_quota_{}_{}".format(cluster, account, topic),
        project_id=solomon_project.id,
        name="Logbroker {} Partition Quota Usage for {}/{}".format(cluster, account, topic),
        annotations={
            "description": "https://logbroker.yandex-team.ru/docs/reference/metrics#PartitionMaxWriteQuotaUsage",
            "usage_percentage": "{{expression.percentage_str}}"
        },
        type=Type(
            expression=Expression(
                program="""
                    let percentage = avg({0}) / {1};
                    let percentage_str = to_fixed(percentage, 2);
                    alarm_if(percentage > 0.95);
                    warn_if(percentage > 0.70);
                """.format(lb_partition_quota_usage_sensor(cluster, account, topic), threshold)
            )
        ),
        window_secs=int(datetime.timedelta(minutes=10).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds()),
    )
    if cluster == "lbk":
        return MultiAlert(
            group_by_labels={
                "host",
                "OriginDC"
            },
            **kwargs
        )
    elif cluster == "lbkx":
        return Alert(
            **kwargs
        )

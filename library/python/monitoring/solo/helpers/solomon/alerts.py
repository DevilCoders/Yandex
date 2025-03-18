# -*- coding: utf-8 -*-
import datetime

from library.python.monitoring.solo.objects.solomon.sensor import Sensor
from library.python.monitoring.solo.objects.solomon.v2 import Alert, Type, Expression
from library.python.monitoring.solo.util.text import drop_spaces


def shard_metrics_limits_alerts(project_id, shard_id, limit_percentage=0.9):
    file_metrics_alert = Alert(
        id="shard_{0}_file_metrics_limit".format(shard_id),
        name="Shard {0} file metrics limit".format(shard_id),
        project_id=project_id,
        type=Type(
            expression=Expression(
                program="""
                let percentage = avg({0}) / avg({1});
                alarm_if(percentage > {2});
                """.format(
                    Sensor(project="solomon", cluster="production", service="coremon", host="Sas",
                           projectId=project_id, shardId=shard_id, sensor="engine.fileSensors"),
                    Sensor(project="solomon", cluster="production", service="coremon", host="Sas",
                           projectId=project_id, shardId=shard_id, sensor="engine.fileSensorsLimit"),
                    limit_percentage
                )
            )
        ),
        annotations={
            "percentage": "{{expression.percentage}}",
            "description": drop_spaces("""
                https://docs.yandex-team.ru/solomon/concepts/limits#shard-limits
                https://docs.yandex-team.ru/solomon/best-practices/quota-monitoring#metrics
            """)
        },
        window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds())
    )

    in_memory_metrics_alert = Alert(
        id="shard_{0}_in_memory_metrics_limit".format(shard_id),
        name="Shard {0} in memory metrics limit".format(shard_id),
        project_id=project_id,
        type=Type(
            expression=Expression(
                program="""
                let percentage = avg({0}) / avg({1});
                alarm_if(percentage > {2});
                """.format(
                    Sensor(project="solomon", cluster="production", service="coremon", host="Sas",
                           projectId=project_id, shardId=shard_id, sensor="engine.inMemSensors"),
                    Sensor(project="solomon", cluster="production", service="coremon", host="Sas",
                           projectId=project_id, shardId=shard_id, sensor="engine.inMemSensorsLimit"),
                    limit_percentage
                )
            )
        ),
        annotations={
            "percentage": "{{expression.percentage}}",
            "description": drop_spaces("""
                https://docs.yandex-team.ru/solomon/concepts/limits#shard-limits
                https://docs.yandex-team.ru/solomon/best-practices/quota-monitoring#metrics
            """)
        },
        window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
        delay_seconds=int(datetime.timedelta(minutes=1).total_seconds())
    )

    return [
        file_metrics_alert,
        in_memory_metrics_alert,
    ]

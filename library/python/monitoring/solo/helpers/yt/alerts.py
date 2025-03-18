# -*- coding: utf-8 -*-
import datetime

from library.python.monitoring.solo.helpers.yt.sensors import \
    chunk_sensor, chunk_limit_sensor, \
    disk_space_sensor, disk_space_limit_sensor, \
    node_sensor, node_limit_sensor, \
    tablet_static_memory_sensor, tablet_static_memory_limit_sensor, \
    tablet_count_sensor, tablet_count_limit_sensor

from library.python.monitoring.solo.objects.solomon.v2 import Alert, Expression, MultiAlert, Type


DEFAULT_WINDOW_SECONDS = int(datetime.timedelta(minutes=5).total_seconds())

DEFAULT_DELAY_SECONDS = int(datetime.timedelta(minutes=1).total_seconds())

ALERT_EXPRESSION = """
    let percentage = avg({quota}) / avg({limit});
    let lastValue = to_fixed(percentage, 2);
    alarm_if(percentage > {threshold_alarm});
    warn_if(percentage > {threshold_warn});
"""


def _format_threshold(threshold):
    return str(int(100 * threshold))


def _format_name(name):
    return name.replace("-", "_")


def yt_chunk_quota_alert(solomon_project, cluster, account, threshold_warn=0.9, threshold_alarm=0.95):
    return Alert(
        id="yt_chunk_{0}_{1}_quota_{2}_{3}".format(cluster, account, _format_threshold(threshold_warn), _format_threshold(threshold_alarm)),
        project_id=solomon_project.id,
        name="YT Chunk quota for account {0} on cluster {1} > {2}".format(account, cluster, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=chunk_sensor(cluster, account),
                    limit=chunk_limit_sensor(cluster, account),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm
                )
            )
        ),
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS
    )


def yt_node_quota_alert(solomon_project, cluster, account, threshold_warn=0.9, threshold_alarm=0.95):
    return Alert(
        id="yt_node_{0}_{1}_quota_{2}_{3}".format(cluster, account, _format_threshold(threshold_warn), _format_threshold(threshold_alarm)),
        project_id=solomon_project.id,
        name="YT Node quota for account {0} on cluster {1} > {2}".format(account, cluster, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=node_sensor(cluster, account),
                    limit=node_limit_sensor(cluster, account),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm
                )
            )
        ),
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS
    )


def yt_tablet_static_memory_alert(solomon_project, cluster, account, threshold_warn=0.9, threshold_alarm=0.95):
    return Alert(
        id="yt_tablet_static_memory_{0}_{1}_quota_{2}_{3}".format(cluster, account, _format_threshold(threshold_warn), _format_threshold(threshold_alarm)),
        project_id=solomon_project.id,
        name="YT Tablet Static Memory quota for account {0} on cluster {1} > {2}".format(account, cluster, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=tablet_static_memory_sensor(cluster, account),
                    limit=tablet_static_memory_limit_sensor(cluster, account),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm
                )
            )
        ),
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS
    )


def yt_tablet_count_alert(solomon_project, cluster, account, threshold_warn=0.9, threshold_alarm=0.95):
    return Alert(
        id="yt_tablet_count_{0}_{1}_quota_{2}_{3}".format(cluster, account, _format_threshold(threshold_warn), _format_threshold(threshold_alarm)),
        project_id=solomon_project.id,
        name="YT Tablet Count quota for account {0} on cluster {1} > {2}".format(account, cluster, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=tablet_count_sensor(cluster, account),
                    limit=tablet_count_limit_sensor(cluster, account),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm
                )
            )
        ),
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS
    )


def yt_disk_space_alert(solomon_project, cluster, account, medium="default", threshold_warn=0.9, threshold_alarm=0.95):
    alert_id = "yt_disk_{0}_{1}_{2}_quota_{3}_{4}".format(
        _format_name(cluster),
        _format_name(account),
        _format_name(medium),
        _format_threshold(threshold_warn), _format_threshold(threshold_alarm))

    return Alert(
        id=alert_id,
        project_id=solomon_project.id,
        name="YT Disk quota for medium {2} for account {0} on cluster {1} > {3}".format(account, cluster, medium, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=disk_space_sensor(cluster, account, medium),
                    limit=disk_space_limit_sensor(cluster, account, medium),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm
                )
            )
        ),
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS
    )


def yt_disk_space_alert_by_medium(solomon_project, cluster, account, threshold_warn=0.9, threshold_alarm=0.95):
    alert_id = "yt_disk_{0}_{1}_quota_{2}_{3}".format(
        _format_name(cluster),
        _format_name(account),
        _format_threshold(threshold_warn),
        _format_threshold(threshold_alarm))

    return MultiAlert(
        id=alert_id,
        project_id=solomon_project.id,
        name="YT Disk quota for account {0} on cluster {1} > {2}".format(account, cluster, threshold_warn),
        type=Type(
            expression=Expression(
                program=ALERT_EXPRESSION.format(
                    quota=disk_space_sensor(cluster, account, "*"),
                    limit=disk_space_limit_sensor(cluster, account, "*"),
                    threshold_warn=threshold_warn,
                    threshold_alarm=threshold_alarm)
            )
        ),
        group_by_labels={"medium"},
        window_secs=DEFAULT_WINDOW_SECONDS,
        delay_seconds=DEFAULT_DELAY_SECONDS,
        annotations={
            "medium": "{{labels.medium}}",
        },
    )

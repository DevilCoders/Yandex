import datetime

from juggler_sdk import Check, Child

from library.python.monitoring.solo.example.example_project.registry.channel.channels import solo_example_channel
from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.example.example_project.registry.sensor.sensors import excosecant, exsecant, cos, sin, cos_sin
from library.python.monitoring.solo.example.example_project.registry.shard.shards import solo_example_pull_shard, solo_example_push_shard
from library.python.monitoring.solo.helpers.solomon.alerts import shard_metrics_limits_alerts
from library.python.monitoring.solo.objects.solomon.v2 import Alert, Type, Threshold, PredicateRule, Expression, MultiAlert


def alert_check(alert):
    return Check(
        host=alert.annotations["host"],
        service=alert.annotations["service"],
        namespace="solo_example_namespace",
        refresh_time=60, ttl=600,
    )


excosecant_modulo_greater_than_40_alert = Alert(
    id="excosecant_modulo_greater_than_40",
    name="Excosecant modulo > 40",
    project_id=solo_example_project.id,
    type=Type(
        threshold=Threshold(
            selectors=excosecant.selectors,
            time_aggregation="MAX",
            predicate="GT",
            threshold=40,
            predicate_rules=[
                PredicateRule(
                    threshold_type="MAX",
                    comparison="GT",
                    threshold=40,
                    target_status="ALARM"
                ),
                PredicateRule(
                    threshold_type="MIN",
                    comparison="LT",
                    threshold=-40,
                    target_status="ALARM"
                ),
            ]
        )
    ),
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={solo_example_channel.id},
    annotations={
        "service": "excosecant_modulo_alert_status",
        "host": "solo_example_project",
    },
)

excosecant_modulo_greater_than_40_check = alert_check(excosecant_modulo_greater_than_40_alert)

exsecant_value_greater_than_15_alert = Alert(
    id="exsecant_value_greater_than_15",
    name="Exsecant value > 15",
    project_id=solo_example_project.id,
    type=Type(
        expression=Expression(
            program=f"""
                        let source = {exsecant};
                        alarm_if(size(source) == 0);
                        let lastValue = max(source);
                        alarm_if(lastValue > 15);
                    """)),
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={solo_example_channel.id},
    annotations={
        "service": "exsecant_value_alert_status",
        "host": "solo_example_project",
    },
)

exsecant_value_greater_than_15_check = alert_check(exsecant_value_greater_than_15_alert)

cos_value_greater_than_1_alert = Alert(
    id="cos_value_greater_than_1",
    name="Cos value > 1",
    project_id=solo_example_project.id,
    type=Type(
        expression=Expression(
            program=f"""
                        let source = {cos};
                        alarm_if(size(source) == 0);
                        let lastValue = max(source);
                        alarm_if(lastValue > 15);
                    """)),
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={solo_example_channel.id},
    annotations={
        "service": "cos_value_alert_status",
        "host": "solo_example_project",
    },
)

cos_value_greater_than_1_check = alert_check(cos_value_greater_than_1_alert)

sin_value_alert = Alert(
    id="sin_value",
    name="Sin value alert",
    project_id=solo_example_project.id,
    type=Type(
        threshold=Threshold(
            selectors=sin.selectors,
            time_aggregation="MAX",
            predicate="GT",
            threshold=0.5,
            predicate_rules=[
                PredicateRule(
                    threshold_type="MAX",
                    comparison="GT",
                    threshold=0.5,
                    target_status="ALARM"
                ),
                PredicateRule(
                    threshold_type="MIN",
                    comparison="LT",
                    threshold=-0.9,
                    target_status="ALARM"
                ),
            ]
        )
    ),
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={solo_example_channel.id},
    annotations={
        "service": "sin_value_alert_status",
        "host": "solo_example_project",
    },
)

sin_value_check = alert_check(sin_value_alert)

cos_sin_alert = MultiAlert(
    id="cos_sin_value",
    project_id=solo_example_project.id,
    name="Cos/Sin value multialert",
    type=Type(
        expression=Expression(
            program=f"""
                    let source = {cos_sin};
                    let lastValue = min(source);
                    alarm_if(lastValue > 1);
                """)),
    group_by_labels={"sensor"},
    notification_channels={solo_example_channel.id},
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    annotations={
        "service": "cos_sin_value_status_by_group",
        "host": "{{labels.sensor}}",
    },
)
cos_sin_check = Check(
    host="solo_example_project",
    service=cos_sin_alert.id,
    namespace="solo_example_namespace",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    children=[Child(
        host="service = {}".format(cos_sin_alert.annotations["service"]),
        service="all",
        instance="all",
        group_type="EVENTS"
    )],
    tags=['solo_example']
)

exports = [
    excosecant_modulo_greater_than_40_alert,
    excosecant_modulo_greater_than_40_check,
    exsecant_value_greater_than_15_alert,
    exsecant_value_greater_than_15_check,
    cos_value_greater_than_1_alert,
    cos_value_greater_than_1_check,
    sin_value_alert,
    sin_value_check,
    cos_sin_alert,
    cos_sin_check
]
exports += shard_metrics_limits_alerts(solo_example_project.id, solo_example_pull_shard.id)
exports += shard_metrics_limits_alerts(solo_example_project.id, solo_example_push_shard.id)

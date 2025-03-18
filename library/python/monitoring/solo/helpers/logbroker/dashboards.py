# -*- coding: utf-8 -*-
from library.python.monitoring.solo.helpers.logbroker.sensors import lb_quota_sensor, lb_bytes_written_sensor, lb_write_lag_sensor
from library.python.monitoring.solo.objects.solomon.v2 import Dashboard, Row, Panel, Parameter
from library.python.monitoring.solo.util.text import underscore


# TODO: test
def lb_dashboard(solomon_project, topics_dict, name="Logbroker Dashboard"):
    """
    :param solomon_project:
    :param topics_dict: {cluster1: [{account1: [topic1, topic2], account2: [topic1, topic2]}]}
    :param name:
    """
    return Dashboard(
        id=underscore(name),
        project_id=solomon_project.id,
        name=name,
        parameters={
            Parameter(name="host", value="Iva|Man|Myt|Sas|Vla"),
            Parameter(name="project", value="kikimr"),
            Parameter(name="cluster", value="lbk")
        },
        rows=[
            Row(
                panels=[
                    Panel(url=lb_quota_sensor(cluster, account, topic).build_sensor_link(full_link=False), title="{} Quota Usage and Limit".format(topic)),
                    Panel(url=lb_bytes_written_sensor(cluster, account, topic).build_sensor_link(full_link=False), title="{} Bytes Written".format(topic)),
                    Panel(url=lb_write_lag_sensor(cluster, account, topic).build_sensor_link(full_link=False), title="{} Write Lag".format(topic)),
                ]
            )
            for cluster, accounts in topics_dict.items() for account, topics in accounts.items() for topic in topics]
    )

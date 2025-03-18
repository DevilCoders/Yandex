from library.python.monitoring.solo.example.example_project.registry.graph.graphs import cos_graph, sin_graph, exsecant_graph, excosecant_graph
from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.example.example_project.registry.sensor.sensors import cos, sin
from library.python.monitoring.solo.helpers.awacs.dashboards import total_dashboard
from library.python.monitoring.solo.objects.solomon.v2 import Dashboard, Row, Panel

solo_example_dashboard = Dashboard(
    id="solo_example_dashboard",
    project_id=solo_example_project.id,
    name="Solo Example Dashboard",
    rows=[
        Row(
            panels=[
                Panel(url=cos_graph.get_dashboard_link(), title=cos_graph.name),
                Panel(url=sin_graph.get_dashboard_link(), title=sin_graph.name),
            ]
        ),
        Row(
            panels=[
                Panel(url=exsecant_graph.get_dashboard_link(), title=exsecant_graph.name),
                Panel(url=excosecant_graph.get_dashboard_link(), title=excosecant_graph.name),
            ]
        ),
        Row(
            panels=[
                Panel(url=sin.build_sensor_link(full_link=False), title=f"{sin.name.capitalize()} sensor"),
                Panel(url=cos.build_sensor_link(full_link=False), title=f"{cos.name.capitalize()} sensor"),
            ]
        )
    ]
)

exports = [
    solo_example_dashboard
]
exports += total_dashboard(
    solomon_project=solo_example_project,
    balancer_id="ads.adfox.ru",
    report_id="engine_handler",
    project_label="adfox"
)

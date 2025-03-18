from library.python.monitoring.solo.example.example_project.registry.dashboard.dashboards import solo_example_dashboard
from library.python.monitoring.solo.example.example_project.registry.graph.graphs import sin_graph, cos_graph
from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.objects.solomon.v2 import Menu, MenuItem

example_menu = Menu(
    id=solo_example_project.id,
    items=[
        MenuItem(
            title="SoloMenu",
            children=[
                MenuItem(
                    title="Dashboards",
                    children=[
                        MenuItem(
                            title="Solo Example Dashboard",
                            url=solo_example_dashboard.get_link()
                        )
                    ]
                ),
                MenuItem(
                    title="Graphs",
                    children=[
                        MenuItem(
                            title="Sin graph",
                            url=sin_graph.get_link()
                        ),
                        MenuItem(
                            title="Cos graph",
                            url=cos_graph.get_link()
                        )
                    ]
                )
            ]
        )
    ]

)

exports = [
    example_menu
]

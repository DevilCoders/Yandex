from library.python.monitoring.solo.objects.solomon.v3 import Dashboard, Widget, ChartWidget

basic_dashboard = Dashboard(
    id="basic_dashboard",
    project_id="solo_example",
    name="basic dashboard",
    description="basic dashboard description",
    title="basic dashboard title",
    widgets=[
        Widget(
            position=Widget.LayoutPosition(x=10, y=10, w=10, h=10),
            chart=ChartWidget(
                id="constant_line_and_integrate",
                title="test widget title",
                queries=ChartWidget.Queries(
                    targets=[
                        ChartWidget.Queries.Target(
                            query="constant_line(10)",
                            text_mode=True,
                            hidden=False,
                        ),
                        ChartWidget.Queries.Target(
                            query="constant_line(15)",
                            text_mode=True,
                            hidden=False,
                        ),
                    ]
                )
            )
        )
    ]
)

exports = [
    basic_dashboard
]

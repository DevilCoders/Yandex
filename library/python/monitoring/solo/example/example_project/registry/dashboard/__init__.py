from itertools import chain

from library.python.monitoring.solo.example.example_project.registry.dashboard.dashboards import exports as dashboards
from library.python.monitoring.solo.example.example_project.registry.dashboard.juggler_dashboard import exports as juggler_dashboard


def get_all_dashboards():
    return list(
        chain(
            dashboards,
            juggler_dashboard,
        )
    )

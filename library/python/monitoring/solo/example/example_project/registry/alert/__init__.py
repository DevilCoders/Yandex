from itertools import chain

from library.python.monitoring.solo.example.example_project.registry.alert.alerts import exports


def get_all_alerts():
    return list(chain(
        exports
    ))

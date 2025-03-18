from itertools import chain

from library.python.monitoring.solo.example.example_project.registry.graph.graphs import exports


def get_all_graphs():
    """
    Provides set of local graphs instances
    :return:
    """
    return list(
        chain(
            exports
        )
    )

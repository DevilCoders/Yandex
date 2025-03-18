from itertools import chain

from library.python.monitoring.solo.example.example_project.registry.menu.menu import exports


def get_all_menu():
    """
    Provides set of local menu instances
    :return:
    """
    return list(
        chain(
            exports
        )
    )

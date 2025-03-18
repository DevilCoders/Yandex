from library.python.monitoring.solo.example.example_project.registry import objects_registry
from library.python.monitoring.solo.helpers.cli import build_basic_cli_v2

if __name__ == "__main__":
    cli = build_basic_cli_v2(
        objects_registry=objects_registry,
        juggler_mark="solo_example_juggler",
        state_file_path="./solo.json",
        solomon_endpoint="http://solomon.yandex.net/api/v2/",
        juggler_endpoint="http://juggler-api.search.yandex.net"
    )
    cli()

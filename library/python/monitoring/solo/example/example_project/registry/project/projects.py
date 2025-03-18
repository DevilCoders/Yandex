from library.python.monitoring.solo.objects.solomon.v2 import Project

# обычно проект не контролируется Соло (проект уже создан), но нужен как объект для связи остальных сущностей
solo_example_project = Project(
    id="solo_example",
    name="Solo Example Project",
    abc_service="adfox",
    only_auth_push=True,
    only_sensor_name_shards=False,
    only_new_format_writes=False,
    only_new_format_reads=False
)

exports = [solo_example_project]

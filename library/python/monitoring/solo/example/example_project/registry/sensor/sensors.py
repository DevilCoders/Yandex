from library.python.monitoring.solo.example.example_project.registry.cluster.clusters import solo_example_cluster
from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.example.example_project.registry.service.services import solo_example_pull_service, solo_example_push_service
from library.python.monitoring.solo.objects.solomon.sensor import Sensor

# объекты сенсоров нужны только для связи с другими сущностями в Соло и не прокидываются в Соломон
exsecant = Sensor(
    project=solo_example_project.id,
    cluster=solo_example_cluster.name,
    service=solo_example_pull_service.name,
    sensor="exsecant",
    kind="DGAUGE"
)

excosecant = Sensor(
    project=solo_example_project.id,
    cluster=solo_example_cluster.name,
    service=solo_example_pull_service.name,
    sensor="excosecant",
    kind="DGAUGE"
)

cos = Sensor(
    project=solo_example_project.id,
    cluster=solo_example_cluster.name,
    service=solo_example_push_service.name,
    sensor="cos",
    kind="DGAUGE"
)

sin = Sensor(
    project=solo_example_project.id,
    cluster=solo_example_cluster.name,
    service=solo_example_push_service.name,
    sensor="sin",
    kind="DGAUGE"
)

cos_sin = Sensor(
    project=solo_example_project.id,
    cluster=solo_example_cluster.name,
    service=solo_example_push_service.name,
    sensor="*",
    kind="DGAUGE"
)

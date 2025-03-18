from library.python.monitoring.solo.example.example_project.registry.cluster.clusters import solo_example_cluster
from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.example.example_project.registry.service.services import solo_example_push_service, solo_example_pull_service
from library.python.monitoring.solo.objects.solomon.v2 import Shard

solo_example_pull_shard = Shard(
    id="solo_example_cluster_pull_service",
    project_id=solo_example_project.id,
    cluster_id=solo_example_cluster.name,
    service_id=solo_example_pull_service.name,
    sensors_ttl_days=3,
    sensor_name_label=""
)

solo_example_push_shard = Shard(
    id="solo_example_cluster_push_service",
    project_id=solo_example_project.id,
    cluster_id=solo_example_cluster.name,
    service_id=solo_example_push_service.name,
    sensors_ttl_days=3,
    sensor_name_label=""
)

# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
    solo_example_pull_shard,
    solo_example_push_shard
]

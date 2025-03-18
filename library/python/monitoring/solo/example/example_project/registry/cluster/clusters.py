from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.objects.solomon.v2 import Cluster, YpCluster

solo_example_cluster = Cluster(
    id="solo_example_cluster",
    project_id=solo_example_project.id,
    name="solo_example_cluster",
    use_fqdn=True,
    sensors_ttl_days=3,
    yp_clusters={
        YpCluster(pod_set_id="solo-dummy-dataprovider.deployUnit", cluster="sas"),
    },
    port=8080
)

# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
    solo_example_cluster
]

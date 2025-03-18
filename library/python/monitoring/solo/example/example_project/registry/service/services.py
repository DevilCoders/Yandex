from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.objects.solomon.v2 import AggrRules
from library.python.monitoring.solo.objects.solomon.v2 import SensorConf
from library.python.monitoring.solo.objects.solomon.v2 import Service


solo_example_pull_service = Service(
    id="solo_example_pull_service",
    project_id=solo_example_project.id,
    name="solo_example_pull_service",
    port=8080,
    path="/sensors/pull",
    interval=60,
    grid=0
)

solo_example_push_service = Service(
    id="solo_example_push_service",
    name="solo_example_push_service",
    project_id=solo_example_project.id,
    interval=60,
    grid=0
)

solo_example_aggregation_rules = Service(
    id="solo_example_aggregation_rules",
    name="solo_example_aggregation_rules",
    project_id=solo_example_project.id,
    interval=60,
    grid=0,
    sensor_conf=SensorConf(
        aggr_rules={
            AggrRules(cond={"host=*"}, target={"host=cluster"}),
            AggrRules(cond={"host=*"}, target={"host={{DC}}"}),

            AggrRules(cond={"unnecessary_label=*"}, target={"unnecessary_label=-"}),

            AggrRules(
                cond={"summable_label=*"},
                target={"summable_label={{summable_label}}"},
                function="SUM",
            ),
            AggrRules(
                cond={"last_label=*"},
                target={"last_label={{last_label}}"},
                function="LAST",
            ),
        },
        priority_rules=set(),
        raw_data_mem_only=False,
    )
)

# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
    solo_example_pull_service,
    solo_example_push_service,
    solo_example_aggregation_rules,
]

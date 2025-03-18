from library.python.monitoring.solo.example.example_project.registry.project.projects import solo_example_project
from library.python.monitoring.solo.objects.solomon.v2 import Channel, Method, Juggler

# через поля host и service связываются между собой алерты в Solomon и чеки в Juggler
solo_example_channel = Channel(
    id="solo_example_channel",
    project_id=solo_example_project.id,
    name="Juggler",
    method=Method(
        juggler=Juggler(
            host="{{{annotations.host}}}",
            service="{{{annotations.service}}}",
            description=""
        )),
    notify_about_statuses={"NO_DATA",
                           "OK",
                           "ERROR",
                           "ALARM",
                           "WARN"},
)

# чтобы быть добавленными в общий regisry, все объекты, которые мы хотим создать/модифицировать должны быть указаны в списке exports
exports = [
    solo_example_channel
]

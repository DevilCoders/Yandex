from library.python.monitoring.solo.objects.juggler import Dashboard, Link, Component, \
    MutesOptions, DowntimesOptions
from library.python.monitoring.solo.util.arcanum import generate_arcanum_url
from library.python.monitoring.solo.helpers.juggler.dashboard import make_components_grid, \
    make_checks_options_from_check
from library.python.monitoring.solo.example.example_project.registry.alert.alerts import \
    cos_sin_check

components = [
    Component(
        name="[Juggler] cos_sin_check",
        elements_in_row=1,
        component_type="AGGREGATE_CHECKS",
        aggregate_checks_options=make_checks_options_from_check(cos_sin_check),
        view_type="DETAILED",
    ),
    Component({
        "name": "[Juggler] Simple notification",
        "elements_in_row": 1,
        "component_type": "NOTIFICATIONS",
        "notifications_options": {
            "filters": [{
                "host": cos_sin_check.host,
                "service": cos_sin_check.service,
            }],
        },
        "view_type": "DETAILED",
    }),
    Component(
        name="[Juggler] Simple mutes",
        elements_in_row=1,
        component_type="MUTES",
        mutes_options=MutesOptions({
            "filters": [{
                "host": cos_sin_check.host,
                "service": cos_sin_check.service,
            }],
        }),
        view_type="DETAILED",
    ),
    Component(
        name="[Juggler] Simple downtimes",
        elements_in_row=1,
        component_type="DOWNTIMES",
        downtimes_options=DowntimesOptions({
            "filters": [{
                "host": cos_sin_check.host,
                "service": cos_sin_check.service,
            }],
        }),
        view_type="DETAILED",
    ),
    Component({
        "name": "[Juggler] Simple aggregate",
        "elements_in_row": 1,
        "component_type": "AGGREGATE_CHECKS",
        "aggregate_checks_options": {
            "filters": [{
                "project": cos_sin_check.namespace,
                "tags": cos_sin_check.tags,
            }],
        },
        "view_type": "DETAILED",
    }),
]

meta_dashboard = Dashboard(
    address='solo_example_dashboard',
    name='Пример дашборда из соло',
    description=f'Дашборд сгенерирован из директории: {generate_arcanum_url()}',
    project='solo_example_project',
    links=[
        Link({
            'title': 'Тикет на разработку',
            'url': 'https://st.yandex-team.ru/SOLO-23',
        }),
        Link({
            'title': 'Чат solo',
            'url': 'https://t.me/joinchat/jMfabQ2WXgg3MTM6'
        }),
    ],
    components=make_components_grid(components, 2),
)

exports = [
    meta_dashboard,
]

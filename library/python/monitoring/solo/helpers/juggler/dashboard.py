from library.python.monitoring.solo.objects.juggler import Component, ChecksOptions


def make_components_grid(components, grid_size=3):
    result = []
    row = 1
    column = 0

    for i, component in enumerate(components):
        if i % grid_size == 0 and i != 0:
            row = row + 1
            column = 1
        else:
            column = column + 1

        result.append(
            Component(component.to_json(), row=row, column=column)
        )

    return result


def make_checks_options_from_check(alert):
    return ChecksOptions({
        "project": alert.namespace,
        "filters": [{
            "host": alert.host,
            "service": alert.service,
        }],
    })


def make_checks_options_from_checks(alerts):
    return ChecksOptions({
        "project": alerts[0].namespace,
        "filters": [{
            "host": alert.host,
            "service": alert.service,
        } for alert in alerts],
    })


def make_checks_options_from_selectors(selectors):
    return ChecksOptions({
        "filters": [{
            "host": selector['host'],
            "service": selector['service'],
        } for selector in selectors],
    })

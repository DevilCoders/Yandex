from collections import OrderedDict
from datetime import datetime

from click import ClickException

from cloud.mdb.cli.dbaas.internal.rest import rest_request
from cloud.mdb.cli.dbaas.internal.utils import get_oauth_token
from juggler_sdk import CheckFilter, DowntimeSearchFilter, DowntimeSelector, JugglerApi

_default = object()


def get_downtime(ctx, downtime_id):
    filters = [DowntimeSearchFilter(downtime_id=downtime_id)]
    with _juggler_api(ctx) as api:
        dowtimes = api.get_downtimes(filters=filters).items

    if not dowtimes:
        raise ClickException(f'Downtime {downtime_id} not found')

    return _format_downtime(dowtimes[0])


def get_downtimes(ctx, *, user=None, namespace=None, hostname=None, service=None, limit=1000):
    filters = []
    filters.append(
        DowntimeSearchFilter(
            user=user,
            namespace=namespace,
            host=hostname,
            service=service,
        )
    )

    with _juggler_api(ctx) as api:
        dowtimes = api.get_downtimes(filters=filters, page_size=limit).items

    return [_format_downtime(dowtime) for dowtime in dowtimes]


def _format_downtime(downtime):
    return OrderedDict(
        (
            ('id', downtime.downtime_id),
            ('description', downtime.description),
            ('start_time', datetime.fromtimestamp(downtime.start_time)),
            ('end_time', datetime.fromtimestamp(downtime.end_time)),
            ('filters', [filter.fields for filter in downtime.filters]),
            ('source', downtime.source),
        )
    )


def create_downtime(ctx, hostnames, service=None, namespace=None, end_time=None, description=None):
    assert len(hostnames) <= 50
    return create_downtimes(ctx, hostnames, service, namespace, end_time, description)


def create_downtimes(ctx, hostnames, service=None, namespace=None, end_time=None, description=None):
    result = []
    with _juggler_api(ctx) as api:
        filters = [DowntimeSelector(host=hostname, service=service, namespace=namespace) for hostname in hostnames]
        for i in range(0, len(filters), 50):
            downtime_id = api.set_downtimes(
                filters=filters[i : i + 50], end_time=end_time.timestamp(), description=description
            ).downtime_id
            result.append(downtime_id)

    return result


def delete_downtimes(ctx, downtime_ids):
    with _juggler_api(ctx) as api:
        api.remove_downtimes(downtime_ids)


def get_check(ctx, hostname, service):
    with _juggler_api(ctx) as api:
        check = api.get_one_check(host=hostname, service=service)

    return _format_check(check)


def get_checks(ctx, namespace=None, hostname=None, service=None):
    filters = [
        CheckFilter(namespace=namespace, host=hostname, service=service),
    ]
    with _juggler_api(ctx) as api:
        checks = api.get_checks(filters=filters).checks

    return [_format_check(check) for check in checks]


def _format_check(check):
    return check.to_dict()


def delete_checks(ctx, namespace=None, hostname=None, service=None):
    filters = [
        CheckFilter(namespace=namespace, host=hostname, service=service),
    ]
    with _juggler_api(ctx) as api:
        return api.remove_checks(filters=filters)


def get_events(ctx, hostname, service=None):
    event_filter = {}
    event_filter['host'] = hostname
    if service:
        event_filter['service'] = service

    response = rest_request(
        ctx,
        'juggler',
        'POST',
        '/v2/events/get_raw_events',
        data={
            'filters': [event_filter],
        },
    )

    last_events = {}
    for event in response['items']:
        service = event['service']
        last_event = last_events.get(service)
        if not last_event or last_event['received_time'] < event['received_time']:
            last_events[service] = event

    return list(last_events.values())


def get_status(ctx, hostname, service, default=_default):
    events = get_events(ctx, hostname, service)
    if events:
        return events[0]['status']

    if default != _default:
        return default

    raise ClickException(f'No events for host "{hostname}" and service "{service}" were found in Juggler.')


def _juggler_api(ctx):
    juggler_config = ctx.obj['config']['juggler']
    token = get_oauth_token(ctx, 'juggler')
    return JugglerApi(juggler_config['api_endpoint'], oauth_token=token)

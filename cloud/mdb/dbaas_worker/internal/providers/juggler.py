"""
Juggler API interaction module
"""
import time
from typing import Optional

from dbaas_common import retry, tracing

from ..exceptions import ExposedException
from .common import Change
from .http import HTTPClient, HTTPErrorHandler

_DT_DESCRIPTION = 'dbaas-worker'


class JugglerApiError(ExposedException):
    """
    Base juggler error
    """


def find_workers_downtime(response: dict, host: str, service: str) -> Optional[str]:
    """
    Find downtime set by worker in get downtimes response
    """
    for downtime in response.get('items', []):
        if downtime.get('description', '') != _DT_DESCRIPTION:
            continue
        for flt in downtime.get('filters', []):
            if flt['host'] == host and flt['service'] == service:
                return downtime['downtime_id']
    return None


class JugglerApi(HTTPClient):
    """
    Juggler provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.enabled = self.config.juggler.enabled
        self.default_duration = self.config.juggler.downtime_hours * 3600
        headers = {
            'Authorization': 'OAuth {token}'.format(token=self.config.juggler.token),
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }
        self._init_session(
            self.config.juggler.url,
            default_headers=headers,
            error_handler=HTTPErrorHandler(JugglerApiError),
        )

    @retry.on_exception(JugglerApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Juggler Downtimes Get')
    def _get_downtime(self, fqdn, service=''):
        """
        Get existing downtime id for host
        """
        tracing.set_tag('juggler.fqdn', fqdn)
        filters = [
            {
                'host': fqdn,
                'instance': '',
                'namespace': '',
                'service': service,
                'tags': [],
            }
        ]
        ret = self._make_request('v2/downtimes/get_downtimes', 'post', data={'filters': filters})
        return find_workers_downtime(ret, fqdn, service)

    @retry.on_exception(JugglerApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Juggler Downtime Exists')
    def downtime_exists(self, fqdn, duration=None, service=''):
        """
        Set downtime on host for duration
        """
        if not self.enabled:
            self.logger.info('Skipping set downtime on %s (juggler disabled)', fqdn)
            return

        tracing.set_tag('juggler.fqdn', fqdn)

        if not duration:
            duration = self.default_duration
        tracing.set_tag('juggler.duration', duration)

        filters = {'host': fqdn}
        if service:
            filters['service'] = service
            tracing.set_tag('juggler.service', service)
        data = {
            'filters': [filters],
            'description': _DT_DESCRIPTION,
        }

        existing = self._get_downtime(fqdn, service=service)
        now = int(time.time())
        data['end_time'] = now + duration

        if existing:
            data['downtime_id'] = existing
        else:
            data['start_time'] = now

        self.add_change(
            Change(
                f'downtime.{fqdn}',
                'set',
                rollback=lambda task, safe_revision: self.downtime_absent(fqdn=fqdn),
                critical=True,
            )
        )
        self._make_request('v2/downtimes/set_downtimes', 'post', data=data)

    @retry.on_exception(JugglerApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Juggler Downtime Absent')
    def downtime_absent(self, fqdn):
        """
        Remove downtime from host
        """
        if not self.enabled:
            self.logger.info('Skipping remove downtime from %s (juggler disabled)', fqdn)
            return

        tracing.set_tag('juggler.fqdn', fqdn)

        existing = self._get_downtime(fqdn)
        if existing:
            self.add_change(Change(f'downtime.{fqdn}', 'remove', rollback=Change.noop_rollback))
            self._make_request('v2/downtimes/remove_downtimes', 'post', data={'downtime_ids': [existing]})

    @retry.on_exception(JugglerApiError, factor=10, max_wait=60, max_tries=6)
    @tracing.trace('Juggler Service Status')
    def get_service_status(self, fqdn, service):
        """
        Get raw check status
        """
        if not self.enabled:
            self.logger.info('Marking %s on %s as explicitly OK (juggler disabled)', service, fqdn)
            return 'OK'

        tracing.set_tag('juggler.fqdn', fqdn)
        tracing.set_tag('juggler.service', service)

        res = self._make_request(
            'v2/events/get_raw_events',
            'post',
            data={
                'filters': [
                    {
                        'host': fqdn,
                        'service': service,
                    }
                ],
            },
        )

        last = {}
        for item in res['items']:
            if item['received_time'] > last.get('received_time', 0):
                last = item

        return last.get('status', 'CRIT')

    def service_not_crit(self, fqdn, service):
        """
        Check if service not in CRIT state
        """
        return self.get_service_status(fqdn, service) != 'CRIT'

import requests
import logging
import time
import re

from ci.tasklet.registry.common.monitoring.infra_check_events.proto import infra_check_events_tasklet
from ci.tasklet.registry.common.monitoring.infra_create.proto import infra_create_pb2
from ci.tasklet.common.proto import service_pb2 as ci


INFRA_BASE_URL = 'https://infra-api.yandex-team.ru/v1/events'
INFRA_EVENT_URL_TEMPLATE = 'https://infra.yandex-team.ru/event/{}'


class Event(object):
    def __init__(self, title, active):
        self.title = title
        self.active = active

    def reset(self):
        self.active = False


class InfraCheckEventsImpl(infra_check_events_tasklet.InfraCheckEventsBase):
    def run(self):
        self.events = {}
        self._update_events_status(self._get_current_events(), not self.input.config.wait_for_finish)
        if self.input.config.time_before_infra_event:
            self._update_events_status(self._get_future_events_(), not self.input.config.wait_for_finish)
        if not self._has_active_events():
            return
        if not self.input.config.wait_for_finish:
            if self.input.config.time_before_infra_event:
                raise Exception(f'There is infra events in {self.input.config.time_before_infra_event} interval for config filters: {self._list_of_infra_events()}')
            else:
                raise Exception(f'There is current infra events for config filters: {self._list_of_infra_events()}')
        while True:
            time.sleep(60)
            self._update_events_status(self._get_current_events(), False)
            if self.input.config.time_before_infra_event:
                self._update_events_status(self._get_future_events_(), False)
            if not self._has_active_events():
                return

    def _get_current_events(self):
        events = []
        for filter in self.input.config.filters:
            events.extend(self._get_current_events_for_filter(filter))
        return events

    def _get_current_events_for_filter(self, filter):
        if not filter.placement.service_id or not filter.placement.environment_id:
            raise Exception('No service_id or environment_id found in filter')
        resp = requests.get(INFRA_BASE_URL + '/current', params={
            'serviceId': filter.placement.service_id,
            'environmentId': filter.placement.environment_id,
        }, headers={
            'Content-Type': 'application/json',
        }).json()
        filteredEvents = []
        for event in resp:
            if not self._filter_by_dc(event, filter.placement.data_centers):
                logging.info(f'Event {event["id"]} filtered by DC')
                continue
            if not self._filter_by_type(event, filter.status.type):
                logging.info(f'Event {event["id"]} filtered by type')
                continue
            if not self._filter_by_severity(event, filter.status.severity):
                logging.info(f'Event {event["id"]} filtered by severity')
                continue
            filteredEvents.append(event)
        return filteredEvents

    def _filter_by_dc(self, event, data_centers):
        if len(data_centers) == 0:
            return True
        for dc in data_centers:
            if event[infra_create_pb2.InfraDataCenter.Name(dc).lower()]:
                return True
        return False

    def _filter_by_type(self, event, type):
        if not type:
            return True
        return event['type'].lower() == infra_create_pb2.InfraType.Type.Name(type).lower()

    def _filter_by_severity(self, event, severity):
        if not severity:
            return True
        return event['severity'].lower() == infra_create_pb2.InfraSeverity.Severity.Name(severity).lower()

    def _list_of_infra_events(self):
        urls = []
        for id, event in self.events.items():
            if not event.active:
                continue
            urls.append(INFRA_EVENT_URL_TEMPLATE.format(id))
        return ','.join(urls)

    def _update_events_status(self, events, mark_actual_as_failed):
        for id in self.events:
            self.events[id].reset()
        for current in events:
            self.events[current['id']] = Event(current['title'], True)
        for id, event in self.events.items():
            progress = ci.TaskletProgress()
            progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
            progress.id = f"event_{id}"
            if event.active:
                progress.progress = 0.0
                if mark_actual_as_failed:
                    progress.status = ci.TaskletProgress.Status.FAILED
                else:
                    progress.status = ci.TaskletProgress.Status.RUNNING
            else:
                progress.progress = 1.0
                progress.status = ci.TaskletProgress.Status.SUCCESSFUL
            progress.text = event.title
            progress.url = INFRA_EVENT_URL_TEMPLATE.format(id)
            progress.module = 'INFRA'
            self.ctx.ci.UpdateProgress(progress)

    def _has_active_events(self):
        for id, event in self.events.items():
            if event.active:
                return True
        return False

    def _get_future_events_(self):
        events = []
        hours_and_mins = re.sub(r'[^\d\s]', '', self.input.config.time_before_infra_event)
        hours, mins = map(int, hours_and_mins.split())
        for filter in self.input.config.filters:
            events.extend(self._get_future_events_for_filter(filter, hours, mins))
        return events

    def _get_future_events_for_filter(self, filter, hours, mins):
        if not filter.placement.service_id or not filter.placement.environment_id:
            raise Exception('No service_id or environment_id found in filter')
        current_timestamp = int(time.time())
        current_timestamp_plus_input_time = current_timestamp + 3600 * hours + 60 * mins
        resp = requests.get(INFRA_BASE_URL, params={
            'serviceId': filter.placement.service_id,
            'environmentId': filter.placement.environment_id,
            'from': current_timestamp,
            'to': current_timestamp_plus_input_time
        }, headers={
            'Content-Type': 'application/json',
        }).json()
        filteredEvents = []
        for event in resp:
            if not self._filter_by_dc(event, filter.placement.data_centers):
                logging.info(f'Event {event["id"]} filtered by DC')
                continue
            if not self._filter_by_type(event, filter.status.type):
                logging.info(f'Event {event["id"]} filtered by type')
                continue
            if not self._filter_by_severity(event, filter.status.severity):
                logging.info(f'Event {event["id"]} filtered by severity')
                continue
            filteredEvents.append(event)
        return filteredEvents

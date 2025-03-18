import json
import time
import requests
import logging

from tasklet.services.ci import get_ci_env
from ci.tasklet.registry.common.monitoring.infra_create.proto import infra_create_tasklet
from ci.tasklet.registry.common.monitoring.infra_create.proto import infra_create_pb2
from ci.tasklet.common.proto import service_pb2 as ci
from tasklet.services.yav.proto import yav_pb2


INFRA_URL = 'https://infra-api.yandex-team.ru/v1/events'
DEFAULT_INFA_OAUTH_TOKEN_KEY = 'infra.token'
INFRA_EVENT_URL_TEMPLATE = 'https://infra.yandex-team.ru/event/{}'
META_ID_FIELD = 'ci_deduplicate_id'


class InfraCreateImpl(infra_create_tasklet.InfraCreateBase):
    def run(self):
        progress_msg = ci.TaskletProgress()
        progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress_msg.progress = 0.0
        progress_msg.ci_env = get_ci_env(self.input.context)
        progress_msg.module = 'INFRA'
        self.ctx.ci.UpdateProgress(progress_msg)

        meta_id = (
            self.input.config.id if self.input.config.id != 0
            else 'flowLaunch: {} job: {}'.format(
                self.input.context.job_instance_id.flow_launch_id,
                self.input.context.job_instance_id.job_id,
            )
        )
        duplicate_infra_event = self._find_duplicate(meta_id)

        if duplicate_infra_event is None:
            logging.info('Creating new infra event')
            response = requests.post(
                INFRA_URL,
                data=self._create_payload(meta_id),
                headers=self._create_headers()
            )
            response.raise_for_status()

            infra_event_id = json.loads(response.text)['id']
            progress_msg.text = 'Created new infra event'
            self.output.infra_identifier.id = infra_event_id
        else:
            logging.info('Found duplicate infra event %s', duplicate_infra_event['id'])
            progress_msg.text = 'Found duplicate infra event'
            self.output.infra_identifier.id = duplicate_infra_event['id']

        progress_msg.progress = 1.0
        progress_msg.status = ci.TaskletProgress.Status.SUCCESSFUL
        progress_msg.url = INFRA_EVENT_URL_TEMPLATE.format(self.output.infra_identifier.id)
        self.ctx.ci.UpdateProgress(progress_msg)

    def _create_headers(self):
        logging.info('Trying to get oauth token from Yav')
        secret_spec = yav_pb2.YavSecretSpec()
        secret_spec.uuid = self.input.context.secret_uid
        secret_spec.key = self.input.config.yav_infra_oauth_token_key or DEFAULT_INFA_OAUTH_TOKEN_KEY
        headers = {
            'Authorization': 'OAuth {}'.format(
                self.ctx.yav.get_secret(secret_spec).secret
            ),
            'Content-Type': 'application/json'
        }
        logging.info('Got oauth token: ***********************************%s', headers['Authorization'][-4:])
        return headers

    def _create_payload(self, meta_id):

        title = self.input.config.textual.title

        flow_type = self.input.context.flow_type
        logging.info("Flow type is %s", flow_type)
        if flow_type == ci.TaskletContext.FlowType.ROLLBACK:
            if "Rollback: " not in title and "Rollback to: " not in title:
                title = "Rollback: " + title

        infra_setup_payload = {
            'serviceId': self.input.config.placement.service_id,
            'title': title,
            'description': self.input.config.textual.description,
            'startTime': int(time.time()),
            'duration': self.input.config.duration,
            'type': infra_create_pb2.InfraType.Type.Name(self.input.config.status.type).lower(),
            'severity': infra_create_pb2.InfraSeverity.Severity.Name(self.input.config.status.severity).lower(),
            'tickets': ', '.join(self.input.config.placement.tickets),
            'meta': {
                META_ID_FIELD: meta_id,
            },
            'sendEmailNotifications': not self.input.config.do_not_send_emails,
            'setAllAvailableDc': False,
        }

        for data_center in self.input.config.placement.data_centers:
            infra_setup_payload[infra_create_pb2.InfraDataCenter.Name(data_center)] = True

        if self.input.config.placement.environment_id:
            infra_setup_payload["environmentId"] = self.input.config.placement.environment_id

        return json.dumps(infra_setup_payload)

    def _find_duplicate(self, meta_id):
        current_time = int(time.time())
        params = {
            'from': current_time,
            'to': current_time,
            'serviceId': self.input.config.placement.service_id,
        }
        ongoing_events = json.loads(requests.get(INFRA_URL, params=params).text)
        for event in ongoing_events:
            if META_ID_FIELD in event.get('meta', dict()):
                if event.get('meta').get(META_ID_FIELD) == meta_id:
                    return event
        return None

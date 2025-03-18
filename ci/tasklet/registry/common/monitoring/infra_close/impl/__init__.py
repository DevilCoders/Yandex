import json
import time
import requests
import logging

from tasklet.services.ci import get_ci_env
from ci.tasklet.registry.common.monitoring.infra_close.proto import infra_close_tasklet
from ci.tasklet.common.proto import service_pb2 as ci
from tasklet.services.yav.proto import yav_pb2


CLOSE_INFRA_URL_TEMPLATE = 'https://infra-api.yandex-team.ru/v1/events/{}'
DEFAULT_INFA_OAUTH_TOKEN_KEY = 'infra.token'
INFRA_EVENT_URL_TEMPLATE = 'https://infra.yandex-team.ru/event/{}'


class InfraCloseImpl(infra_close_tasklet.InfraCloseBase):
    def run(self):
        progress_msg = ci.TaskletProgress()
        progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress_msg.progress = 0.0
        progress_msg.ci_env = get_ci_env(self.input.context)
        progress_msg.module = 'INFRA'
        self.ctx.ci.UpdateProgress(progress_msg)

        url = CLOSE_INFRA_URL_TEMPLATE.format(self.input.infra_identifier.id)
        payload = {
            'finishTime': int(time.time())
        }
        response = requests.put(url, headers=self._create_headers(), data=json.dumps(payload))
        response.raise_for_status()

        progress_msg.progress = 1.0
        progress_msg.status = ci.TaskletProgress.Status.SUCCESSFUL
        progress_msg.url = INFRA_EVENT_URL_TEMPLATE.format(self.input.infra_identifier.id)
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

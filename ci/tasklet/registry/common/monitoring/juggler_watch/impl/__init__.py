import json
import logging
import requests
import time
from tasklet.services.ci import get_ci_env

from ci.tasklet.common.proto import service_pb2 as ci
from ci.tasklet.registry.common.monitoring.juggler_watch.proto import juggler_watch_tasklet

CHECK_INTERVAL_SECONDS = 10
CHECKS_STATE_URL = 'http://juggler-api.search.yandex.net/v2/checks/get_checks_state'
HEADERS = {'Content-type': 'application/json'}


class JugglerWatchImpl(juggler_watch_tasklet.JugglerWatchBase):
    def run(self):

        start_time = time.time()
        finish_time = (
            start_time + (self.input.config.duration_minutes + self.input.config.delay_minutes) * 60
        )

        progress_msg = self._update_progress(
            start_time,
            finish_time,
            text='Started. Waiting delay {} minutes'.format(self.input.config.delay_minutes),
        )

        time.sleep(self.input.config.delay_minutes * 60)

        progress_msg = self._update_progress(
            start_time,
            finish_time,
            progress_msg=progress_msg,
            text='Started polling monitorings',
        )

        post_data = self._create_post_data()

        logging.info(post_data)

        first_attempt = True
        while time.time() < finish_time or first_attempt:
            first_attempt = False

            resp = self._make_request(post_data)
            self._check_response(resp)

            logging.info('Sleep %s seconds', CHECK_INTERVAL_SECONDS)

            progress_msg = self._update_progress(
                start_time,
                finish_time,
                progress_msg=progress_msg,
            )
            time.sleep(CHECK_INTERVAL_SECONDS)

        self._update_progress(
            start_time,
            finish_time,
            status=ci.TaskletProgress.Status.SUCCESSFUL,
        )

    def _create_post_data(self):
        post_data = {'filters': []}

        for filter_group in self.input.config.filters:
            new_filter = {}
            if filter_group.namespace:
                new_filter['namespace'] = filter_group.namespace
            if filter_group.service:
                new_filter['service'] = filter_group.service
            if filter_group.host:
                new_filter['host'] = filter_group.host
            if filter_group.tags:
                new_filter['tags'] = [tag for tag in filter_group.tags]
            post_data['filters'].append(new_filter)

        return json.dumps(post_data)

    @staticmethod
    def _make_request(post_data, retries=5):
        ret = 0
        while True:
            logging.info('Requesting filters from %s, post_data: %s', CHECKS_STATE_URL, post_data)
            try:
                return requests.post(CHECKS_STATE_URL, data=post_data, headers=HEADERS)
            except:
                logging.exception('Error when checking Juggler status')
                ret += 1
                if ret >= retries:
                    raise

    def _check_response(self, resp):
        fail_statuses = ('CRIT', 'WARN') if self.input.config.fail_on_warn else ('CRIT', )
        if resp.status_code == 200:
            sensors_data = json.loads(resp.text)['items']

            if not sensors_data:
                raise Exception('No monitorings found by filters')

            logging.info('Found %s monitorings', len(sensors_data))
            logging.debug('Found monitorings: %s', sensors_data)

            for sensor in sensors_data:
                if sensor['status'] in fail_statuses:
                    raise Exception('Monitoring failed: %s', sensor)

            logging.info('All monitorings ok')
        else:
            raise Exception(
                'Bad Juggler request status code: %s, %s',
                resp.status_code,
                resp.text,
            )

    def _generate_url(self):
        _or = '%7C'
        _and = '%26'
        _is = '%3D'
        url_template = 'https://juggler.yandex-team.ru/aggregate_checks/?query={}'
        filter_groups = []
        for filter_group in self.input.config.filters:
            filters = []
            if filter_group.namespace:
                filters.append('namespace{}{}'.format(_is, filter_group.namespace))
            if filter_group.service:
                filters.append('service{}{}'.format(_is, filter_group.service))
            if filter_group.host:
                filters.append('host{}{}'.format(_is, filter_group.host))
            if filter_group.tags:
                for tag in filter_group.tags:
                    filters.append('tag{}{}'.format(_is, tag))
            filter_groups.append('({})'.format(_and.join(filters)))
        return url_template.format(_or.join(filter_groups))

    def _update_progress(self, start_time, finish_time, progress_msg=None, text=None, status=None):
        if progress_msg is None:
            progress_msg = ci.TaskletProgress()
            progress_msg.job_instance_id.CopyFrom(self.input.context.job_instance_id)
            progress_msg.ci_env = get_ci_env(self.input.context)
            progress_msg.module = 'JUGGLER'

        total_time = float(max(finish_time - start_time, 1))
        progress_msg.progress = min(float(time.time() - start_time) / total_time, 1.0)
        if text:
            progress_msg.text = text
        if status:
            progress_msg.status = status
        progress_msg.url = self._generate_url()
        self.ctx.ci.UpdateProgress(progress_msg)
        return progress_msg

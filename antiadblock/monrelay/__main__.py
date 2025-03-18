import time
import json
import argparse
from collections import defaultdict
from multiprocessing.dummy import Pool as ThreadPool, JoinableQueue
from Queue import Empty

import requests

from yt.wrapper import YtClient
from library.python.yt import Lock
from tvmauth import TvmClient, TvmApiClientSettings

from antiadblock.monrelay.data_sources.solomon import DSSolomon
from antiadblock.monrelay.data_sources.dsyql import DSYql
from antiadblock.monrelay.data_sources.stat import DSStat
from antiadblock.monrelay.jslogger import JSLogger


INF = float('Inf')


def timeit(func):
    def wrapped(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        duration = time.time() - start
        logger.info('', function=func.__name__, duration=duration)
        return result

    return wrapped


def retry(func):
    delays = (0, 1, 5, None)

    def wrapped(*args, **kwargs):
        for delay in delays:
            try:
                return func(*args, **kwargs)
            except Exception:
                logger.error('Error', exc_info=True)
                if delay is None:
                    raise
                else:
                    time.sleep(delay)

    return wrapped


class ConfigsApiError(Exception):
    pass


@retry
@timeit
def get_services_from_configs_api(tvm_ticket):
    """
    :return: List of active services from configs_api
    """
    response = requests.get(SERVICE_IDS_URL, headers={'X-Ya-Service-Ticket': tvm_ticket})
    if response.status_code != 200:
        raise ConfigsApiError('{code} {text}'.format(code=response.status_code, text=response.text))
    return list(response.json().keys())


@retry
@timeit
def get_checks_configs(tvm_ticket):
    response = requests.get(CHECKS_CONFIGS_URL, headers={'X-Ya-Service-Ticket': tvm_ticket})
    if response.status_code != 200:
        raise ConfigsApiError('{code} {text}'.format(code=response.status_code, text=response.text))
    return response.json()['groups']


@retry
@timeit
def post_checks_results(json_result, tvm_ticket):
    response = requests.post(CHECKS_RESULT_URL, json=json_result, headers={'X-Ya-Service-Ticket': tvm_ticket})
    if response.status_code not in (200, 201):
        raise ConfigsApiError('{code} {text}'.format(code=response.status_code, text=response.text))
    return


def ensure_yt_path(yt_client, lock_dir):
    with yt_client.Transaction():
        if not yt_client.exists(lock_dir):
            try:
                yt_client.create("map_node", lock_dir, recursive=True, ignore_existing=False)
            except Exception:
                logger.error('failed to create lock dir', exc_info=True)


def get_data_source(**kwargs):
    check_type = kwargs['config']['check_type']
    if check_type == 'solomon':
        return DSSolomon(**kwargs)
    elif check_type == 'yql':
        return DSYql(**kwargs)
    elif check_type == 'stat':
        return DSStat(**kwargs)
    else:
        raise Exception('Data Source {} not implemented'.format(kwargs['config']['check_type']))


class MonrelayRunner(object):

    def __init__(self, tvm_settings, yt_client, logger):
        self.__queue = JoinableQueue()
        self.__workers_in_progress = set()
        self.__yt_client = yt_client
        self.tvm_client = TvmClient(tvm_settings)
        self.__logger = logger

    @property
    def checks_last_updated(self):
        return json.loads(self.__yt_client.get(LOCK_DIR + '/checks_last_updated'))

    @checks_last_updated.setter
    def checks_last_updated(self, value):
        last_updated = json.loads(self.__yt_client.get(LOCK_DIR + '/checks_last_updated'))
        last_updated.update(value)
        self.__yt_client.set(LOCK_DIR + '/checks_last_updated', json.dumps(last_updated))

    def run(self):
        """
        One iteration of Monrelay run:
        1. Get DataSource configs (One DS per check_id)
        2. Create async threads for checks running
        3. Get results from queue
        4. Filter only successful checks
        5. POST results to configs_api
        :return: None
        """

        def safe_request(data_source_args):
            """
            Wrapper for DataSource.get_check_result()
            Puts results into queue
            :param data_source_args: DataSource __init__ args
            :return: None
            """
            try:
                result = get_data_source(**data_source_args).get_check_result()
                self.__queue.put({'check_id': data_source_args['config']['check_id'],
                                  'result': result,
                                  'success': True,
                                  'updated': time.time()})
            except Exception:
                conf = data_source_args['config']
                self.__logger.exception(
                    'DS failed: check_type={} check_id={}'.format(conf.get('check_type', ''), conf.get('check_id', '')))
                self.__queue.put({'check_id': data_source_args['config']['check_id'],
                                  'success': False,
                                  })

        data_source_configs = self.get_data_sources_configs()
        if data_source_configs:
            pool = ThreadPool(min(len(data_source_configs), MAX_THREADS))
            pool.map_async(lambda kwargs: safe_request(kwargs), data_source_configs)
            self.__workers_in_progress |= set([c['config']['check_id'] for c in data_source_configs])
            pool.close()
        completed_checks = list()
        try:
            while True:
                completed_checks.append(self.__queue.get_nowait())
        except Empty:
            if not completed_checks:
                return
            success_checks = filter(lambda c: c['success'], completed_checks)
            self.checks_last_updated = {check['check_id']: check['updated'] for check in success_checks}
            self.__workers_in_progress -= set([c['check_id'] for c in completed_checks])
            results = filter(lambda x: x, [result for check_results in map(lambda c: c['result'], success_checks) for result in check_results])

            # Will POST checks to configs_api in separate threads by service_id
            post_data = defaultdict(list)
            for r in results:
                post_data[r['service_id']].append(r)

            def safe_post(result):
                try:
                    post_checks_results(json_result=result, tvm_ticket=self.tvm_client.get_service_ticket_for('configs_api'))
                except ConfigsApiError as e:
                    self.__logger.exception(e)

            ThreadPool(min(len(post_data), MAX_THREADS)).map(safe_post, post_data.values())

    def get_data_sources_configs(self):
        """
        Get check configs and service ids from configs_api
        Transform check configs into DataSource configs (add service_ids, tokens, logger)
        Filter configs checks with expired timeout and those who not in progress now
        :return: Dict of data_sources_configs
        """
        service_ids = get_services_from_configs_api(tvm_ticket=self.tvm_client.get_service_ticket_for('configs_api'))
        checks_configs = get_checks_configs(tvm_ticket=self.tvm_client.get_service_ticket_for('configs_api'))
        checks_timeouts = {check_id: (time.time()-last_updated) for check_id, last_updated in self.checks_last_updated.items()}
        data_source_configs = [
            dict(service_ids=service_ids,
                 oauth_token=TOKENS.get(check['check_type'], None),
                 logger=JSLogger('data_source', logs_path=LOG_PATH, group=group['group_title'],
                                 check_type=check['check_type'], check_title=check['check_title']),
                 config=dict([('group_id', group['group_id'])] + check.items()),
                 )
            for group in checks_configs for check in group['checks']
            if check.pop('update_period') < checks_timeouts.get(check['check_id'], INF) and check['check_id'] not in self.__workers_in_progress]
        self.__logger.info(checks_timeouts)
        return data_source_configs


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--cfg', dest='cfg', required=True, help='path to configuration file')

    args = parser.parse_args()
    with open(args.cfg) as f:
        cfg = json.loads(f.read())

    LOG_PATH = cfg.get('log_path', None)
    YT_TOKEN = cfg['yt_token']
    YQL_TOKEN = cfg['yql_token']

    MONRELAY_TVM_ID = int(cfg['monrelay_tvm_id'])
    MONRELAY_TVM_SECRET = cfg['monrelay_tvm_secret']
    CONFIGSAPI_TVM_ID = int(cfg['configs_api_tvm_id'])

    CONFIGSAPI_URL = 'https://{}/'.format(cfg.get('configs_api_host', 'api.aabadmin.yandex.ru'))
    CHECKS_CONFIGS_URL = CONFIGSAPI_URL + 'get_checks_config'
    CHECKS_RESULT_URL = CONFIGSAPI_URL + 'post_service_checks'
    SERVICE_IDS_URL = CONFIGSAPI_URL + 'v2/configs?status=active&monitorings_enabled=true'
    # key must be a valid check type
    TOKENS = dict(
        solomon=YT_TOKEN,  # fits instead solomon_oauth_token
        yql=YQL_TOKEN,
        stat=YQL_TOKEN
    )

    LOCK_DIR = '//home/antiadblock/monrelay-{}'.format(cfg.get('key', 'test'))
    UPDATE_PERIOD = 30
    MAX_THREADS = 20

    tvm_settings = TvmApiClientSettings(
        self_tvm_id=MONRELAY_TVM_ID,
        self_secret=MONRELAY_TVM_SECRET,
        enable_service_ticket_checking=False,
        dsts=dict(configs_api=CONFIGSAPI_TVM_ID)
    )
    TVM = TvmClient(tvm_settings)

    logger = JSLogger('root', logs_path=LOG_PATH)

    yt_client = YtClient(proxy='locke', token=YT_TOKEN)
    ensure_yt_path(yt_client, LOCK_DIR)
    ensure_yt_path(yt_client, '/'.join([LOCK_DIR, 'lock']))
    checks_last_updated_path = LOCK_DIR + '/checks_last_updated'
    if not yt_client.exists(checks_last_updated_path):
        yt_client.create('document', checks_last_updated_path, ignore_existing=False)
        yt_client.set(checks_last_updated_path, json.dumps({}))

    while True:
        start_ts = time.time()
        end_ts = None
        logger.info("trying to acquire lock")
        try:
            with Lock(LOCK_DIR, client=yt_client, transaction_kwargs={'timeout': 10000}) as lock:
                monrelay = MonrelayRunner(yt_client=yt_client, tvm_settings=tvm_settings, logger=logger)
                while lock.is_lock_pinger_alive():
                    logger.info("lock acquired")
                    if end_ts is not None and end_ts - start_ts < UPDATE_PERIOD:
                        time.sleep(UPDATE_PERIOD - (end_ts - start_ts))
                    start_ts = time.time()
                    monrelay.run()
                    end_ts = time.time()
        except Exception:
            logger.error('Exception occured', exc_info=True)
            time.sleep(5)

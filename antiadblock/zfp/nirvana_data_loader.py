import os
import csv
import json
import logging
import argparse
import subprocess
from datetime import datetime
from collections import OrderedDict

from razladki_wow.core.processor import Task
from razladki_wow.core.datatypes import TimeInterval
from razladki_wow.engines.solomon.processor import SolomonProcessor, LoadPath, get_solomon_host
from razladki_wow.engines.solomon.storage import SolomonStorage, NoSuchSensor


OLD_SERVICE_IDS = "service_id=*"
# TODO: get service_ids from configs_api
REPLACED_SERVICE_IDS = "service_id=autoru|docviewer.yandex.ru|kinopoisk.ru|turbo.yandex.ru|yandex_images|yandex_player|yandex_afisha|zen.yandex.ru" \
                       "|yandex_video|yandex_tv|yandex_sport|yandex_realty|yandex_pogoda|yandex_afisha|yandex_news|yandex_morda|yandex_mail|games.yandex.ru" \
                       "|drive2|echomsk|gorod_rabot|liveinternet|livejournal|nova.rambler.ru|otzovik|razlozhi|smi24"


class SolomonStoragePatch(SolomonStorage):

    def _load_data_for_subalert(self, load_paths, time_interval, multi_keys, multi_values):
        try:
            return super()._load_data_for_subalert(load_paths, time_interval, multi_keys, multi_values)
        except NoSuchSensor as e:
            logging.error(str(e))
            return OrderedDict()


class SolomonProcessorPatch(SolomonProcessor):

    def load_data(self):
        url = get_solomon_host(self.task.description.host)
        storage = SolomonStoragePatch(
            url=url,
            project=self.task.description.project,
            cluster=self.task.description.cluster,
            service=self.task.description.service,
        )

        self.data = storage.load(
            load_paths=self.load_paths,
            time_interval=self.data_interval,
            multi_labels=self.task.description.multi_sel,
            params=self.task.params,
        )

        # Run code even if no data is loaded, e.g. for code debug purposes
        # This will create a single subalert named "alert"
        if not self.data:
            self.data['alert'] = OrderedDict()


class Description:
    """
        Mock for Task creation
    """
    def __init__(self, host, project, cluster, service, multi_sel):
        self.host = host
        self.project = project
        self.cluster = cluster
        self.service = service
        self.multi_sel = multi_sel


def get_secs_from_period(period):
    """
    >> > [get_secs_from_period(n) for n in [1s, 10s, 1m, 15m, 1h, 3h, 1d, 5d, 1w, 4w]]
    [1, 10, 60, 900, 3600, 10800, 86400, 432000, 604800, 2419200]
    """
    multiplier_map = {
        's': 1,
        'm': 60,
        'h': 3600,
        'd': 86400,
        'w': 604800,
    }
    value = int(period[:-1])
    _type = period[-1:]
    return value * multiplier_map[_type]


def log_event(event_msg):
    logging.error("\n{} {}\n".format(datetime.now().strftime('%Y-%m-%dT%H:%M:%S'), event_msg))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--task_json', required=True, help='Path to json with task parameters')
    parser.add_argument('--ts_from', required=True, help='Data period start (timestamp)')
    parser.add_argument('--ts_to', required=True, help='Data period end (timestamp)')
    parser.add_argument('--task_program', required=True, help='Path to program.saql')
    parser.add_argument('--output', required=True, help='Path to save results (folder)')
    parser.add_argument('--alert_id', required=True, help='Currently calculated alert')
    args = parser.parse_args()

    with open(args.task_json, 'r') as f:
        task_json = json.loads(f.read())

    task_json['description']['multi_sel'] = task_json['description']['multi_sel'].replace(OLD_SERVICE_IDS, REPLACED_SERVICE_IDS)
    for i in range(len(task_json['params'])):
        task_json['params'][i] = task_json['params'][i].replace(OLD_SERVICE_IDS, REPLACED_SERVICE_IDS)

    # Create my own Task with only fields necessary for data loading
    task = Task(Description(**task_json['description']), params=task_json['params'])
    processor = SolomonProcessorPatch(task)

    # Calculate load_paths from alert program
    cmd = [
        os.getenv('JAVA_BINARY_PATH'),
        '--enable-preview',
        '-Dorg.slf4j.simpleLogger.defaultLogLevel=error',
        '-Dru.yandex.solomon.LabelValidator=skip',
        '-cp', os.getenv('SOLOMON_CALCULATOR_CLASSPATH'),
        'ru.yandex.solomon.calculator.Prefetcher',
        args.task_program,
    ]
    # Run Solomon calculator
    log_event("start loading path")
    completed_process = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    log_event("end loading path")
    if completed_process.returncode != 0:
        logging.error('Error while calculating load paths')
        logging.error('stderr: ')
        logging.error(completed_process.stderr)
        exit(1)

    # And parse load paths from its output
    load_paths = [line.split(maxsplit=1) for line in completed_process.stdout.decode().splitlines()]
    for i in range(len(load_paths)):
        load_paths[i][0] = load_paths[i][0].replace(OLD_SERVICE_IDS, REPLACED_SERVICE_IDS)
    logging.error(load_paths)
    load_paths = [LoadPath(path, is_single == "1") for path, is_single in load_paths]
    processor.load_paths = load_paths

    # load more data for evaluate first points
    ts_from = int(args.ts_from) - get_secs_from_period(task_json.get("period", "7d"))
    processor.data_interval = TimeInterval(datetime.fromtimestamp(ts_from),
                                           datetime.fromtimestamp(int(args.ts_to)))
    log_event("start loading data")
    processor.load_data()
    log_event("end loading data")

    # Export loaded data to tsv files
    log_event("start export data")
    exported = processor.export_data()

    for selector, data in exported.items():
        # Subalert contains multiselectors we'll use it for filename
        labels = selector.split(processor.SUBALERT_DELIMITER)[0].split('&')
        # service_id first (need for fraud)
        labels = {label.split('=')[0]:label.split('=')[1] for label in labels}
        service_id = labels.pop('service_id', None)
        labels = list(labels.values()) if service_id is None else [service_id] + list(labels.values())
        alert_service = '_'.join(labels)
        alert_host = args.alert_id
        filename = os.path.join(args.output, f'{alert_host}__{alert_service}.tsv')

        with open(filename, 'a') as f:
            writer = csv.writer(f, delimiter='\t')
            writer.writerow(['Date', selector])
            data = [(int(r[0]), r[1]) for r in data]
            writer.writerows(data)

    log_event("end export data")

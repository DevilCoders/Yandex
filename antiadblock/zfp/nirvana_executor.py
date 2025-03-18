import os
import json
import logging
import argparse
import subprocess
from datetime import datetime
from multiprocessing import Pool


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_dir', required=True, help='Path to dir with data for calculation')
    parser.add_argument('--output_dir', required=True, help='Path to dir for saving calculation result')
    parser.add_argument('--programs_dir', required=True, help='Path to dir with programs')
    parser.add_argument('--alert_ids', nargs='+', required=True, help='List alerts')

    args = parser.parse_args()
    calculate_paths = []
    for alert_id in args.alert_ids:
        logging.info(alert_id)
        logging.info(os.listdir(os.path.join(args.input_dir, alert_id)))
        for path in os.listdir(os.path.join(args.input_dir, alert_id)):
            calculate_paths.append((path, alert_id))

    def calculate(params):
        input_file, alert_id = params
        with open(os.path.join(args.programs_dir, alert_id, 'task.json')) as f:
            task_json = json.load(f)

        task_program = os.path.join(args.programs_dir, alert_id, 'program.saql')
        cmd = [
            os.getenv('JAVA_BINARY_PATH'),
            '--enable-preview',
            '-Xms1700m',
            '-Xmx1700m',
            '-XX:+AlwaysPreTouch',
            '-Dorg.slf4j.simpleLogger.defaultLogLevel=error',
            '-Dru.yandex.solomon.LabelValidator=skip',
            '-cp', os.getenv('SOLOMON_CALCULATOR_CLASSPATH'),
            'ru.yandex.solomon.calculator.Main',
            '-i', '{}/{}/{}'.format(args.input_dir, alert_id, input_file),
            '-p', task_program,
            '-o', '{}/{}/{}'.format(args.output_dir, alert_id, input_file),
            '-w', task_json['period'],
            '-q', '60s',
            '-r', '$status'
        ]
        if 'delaySeconds' in task_json:
            cmd += ['-d', str(task_json['delaySeconds']) + 's']

        # Run Solomon calculator
        start_time = datetime.now()
        logging.error("\n{} start calculating for file {}\n".format(start_time.strftime('%Y-%m-%dT%H:%M:%S'), input_file))
        completed_process = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        end_time = datetime.now()
        delta = (end_time - start_time).total_seconds()
        logging.error("\n{} end calculating for file {}, total sec: {} \n".format(end_time.strftime('%Y-%m-%dT%H:%M:%S'), input_file, delta))
        if completed_process.returncode != 0:
            logging.error('Error while calculating razladki for file {}'.format(input_file))
            logging.error('stderr: ')
            logging.error(completed_process.stderr)
            # TODO: raise exception
            # exit(1)

    with Pool(16) as p:
        p.imap(calculate, calculate_paths)
        p.close()
        p.join()

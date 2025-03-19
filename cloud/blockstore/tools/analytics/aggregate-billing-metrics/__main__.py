import concurrent.futures
import json
import logging
import os


from multiprocessing import Lock
from tabulate import tabulate


logger = logging.getLogger()
stream_handler = logging.StreamHandler()
logger.addHandler(stream_handler)
logger.setLevel(logging.INFO)


billing = dict()
lock = Lock()


def process_log(log_name):
    with open(log_name, 'r') as f:
        for line in f.readlines():
            data = json.loads(line)

            schema = data['schema']
            assert schema == 'nbs.volume.allocated.v1'

            usage = data['usage']
            usage_quantity = usage['quantity']
            usage_type = usage['type']
            assert usage_type == 'delta'

            usage_unit = usage['unit']
            assert usage_unit == 'seconds'

            cloud_id = str(data['cloud_id'])

            tags = data['tags']

            disk_type = tags['type']
            assert disk_type in ['network-ssd', 'network-hdd']

            state = tags['state']
            if state == 'offline':
                continue
            assert state == 'online'

            size = tags['size']

            with lock:
                if cloud_id not in billing:
                    billing[cloud_id] = {
                        'network-ssd': 0,
                        'network-hdd': 0
                    }

                billing[cloud_id][disk_type] += size * usage_quantity


def main():
    with concurrent.futures.ThreadPoolExecutor(max_workers=16) as executor:
        future2log = dict()

        for az in ['vla', 'sas', 'myt']:
            for control_id in range(1, 4):
                nbs_control_name = 'nbs-control-{}{}.svc.cloud.yandex.net'.format(az, control_id)
                log_prefix = os.path.join(nbs_control_name, 'metering', 'nbs-metering.log')

                for log_id in range(11):
                    log_name = log_prefix

                    if log_id != 0:
                        log_name = log_prefix + '.%s' % log_id

                    future = executor.submit(process_log, log_name)
                    future2log[future] = log_name

        for future in concurrent.futures.as_completed(future2log):
            log_name = future2log[future]
            try:
                future.result()
            except Exception as e:
                logger.error('[ERR] Couldn\'t process %s: %s', log_name, e)
            else:
                logger.info('[OK] Processed %s', log_name)

        dumped_billing = []
        for k, v in billing.iteritems():
            for _k, _v in v.iteritems():
                dumped_billing += [[k, _k, int(_v / 3600.0)]]

        print(tabulate(dumped_billing, headers=['cloud_id', 'disk_type', 'bytes_hours'], tablefmt='psql'))

if __name__ == "__main__":
    main()

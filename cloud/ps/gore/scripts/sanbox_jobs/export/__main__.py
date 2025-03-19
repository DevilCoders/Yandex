import os
import logging
import time
import requests
import concurrent.futures
from cloud.ps.gore.scripts.sanbox_jobs.config import SOLOMON_TOKEN
from cloud.ps.gore.scripts.sanbox_jobs.src.kicker import renew_apis


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s:%(message)s')
    services = os.getenv('GORE_SERVICES').split()
    errors = {}
    solomonLs = "http://solomon.yandex.net/api/v2/push?project=ycgore&service=lifesupport&cluster=service"
    solomonMetrics = "http://solomon.yandex.net/api/v2/push?project=ycgore&service=export&cluster=service"
    headers = {"Authorization": "OAuth %s" % SOLOMON_TOKEN}

    if services:
        with concurrent.futures.ThreadPoolExecutor(max_workers=5) as executor:
            futures = []
            for service in services:
                futures.append(executor.submit(renew_apis, service))
            for future in concurrent.futures.as_completed(futures):
                service, status_code = future.result()
                errors[service] = status_code

        if errors:
            requests.post(solomonLs, json={"sensors": [{"labels": {"sensor": "heartbeat"}, "ts": int(time.time()), "value": 1}]}, headers=headers)
            solomonRequest = {"sensors": []}
            for service, error in errors.items():
                solomonRequest["sensors"].append({"labels": {"sensor": service}, "ts": int(time.time()), "value": int(error != 200)})
            requests.post(solomonMetrics, json=solomonRequest, headers=headers)
        else:
            requests.post(solomonLs, json={"sensors": [{"labels": {"sensor": "heartbeat"}, "ts": int(time.time()), "value": 0}]}, headers=headers)
            raise KeyError("Error dict is empty. Probably GoRe is down.")

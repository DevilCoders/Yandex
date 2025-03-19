import time
import logging
import requests
from cloud.ps.gore.scripts.sanbox_jobs.config import SOLOMON_TOKEN
from cloud.ps.gore.scripts.sanbox_jobs.src.fetcher import fetch_services
from cloud.ps.gore.scripts.sanbox_jobs.src.kicker import renew_data

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s:%(message)s')
    services = fetch_services()
    errors = {}
    solomonLs = "http://solomon.yandex.net/api/v2/push?project=ycgore&service=lifesupport&cluster=service"
    solomonMetrics = "http://solomon.yandex.net/api/v2/push?project=ycgore&service=import&cluster=service"
    headers = {"Authorization": "OAuth %s" % SOLOMON_TOKEN}

    if services:
        for service in services:
            ans = renew_data(service)
            errors[service] = ans
            time.sleep(0.5)

        if errors:
            requests.post(solomonLs, json={"sensors": [{"labels": {"sensor": "heartbeat"}, "ts": int(time.time()), "value": 1}]}, headers=headers)
            solomonRequest = {"sensors": []}
            for service, error in errors.items():
                solomonRequest["sensors"].append({"labels": {"sensor": service}, "ts": int(time.time()), "value": int(error != 200)})
            requests.post(solomonMetrics, json=solomonRequest, headers=headers)
        else:
            requests.post(solomonLs, json={"sensors": [{"labels": {"sensor": "heartbeat"}, "ts": int(time.time()), "value": 0}]}, headers=headers)
            raise KeyError("Error dict is empty. Probably GoRe is down.")

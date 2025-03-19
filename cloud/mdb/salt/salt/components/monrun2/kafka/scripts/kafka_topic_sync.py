#!/usr/bin/python3

import json
import sys
import requests
import time


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    if "{{ salt.pillar.get('data:kafka:sync_topics') }}" != "True":
        die(0, "Topic synchronization is turned off")

    message = 'OK'
    try:
        resp = requests.get("http://localhost:5000/status")
        resp.raise_for_status()
        status = json.loads(resp.text)
        topic_sync_status = status.get('topic_sync', {})
        ok = topic_sync_status.get('ok', False)
        if not ok:
            errors = []

            metric = topic_sync_status.get('last_healthy_checkpoint_ts')
            if metric:
                duration = int(time.time()) - int(metric)
                errors.append(f'Unhealthy for {duration} seconds')

            if not errors:
                errors.append('Some unknown problem')
            die(2, ', '.join(errors))

        if topic_sync_status.get('local_broker_is_controller', None) is False:
            message = "Topic sync is paused because local broker is not a controller"
    except Exception:
        die(2, "Failed to receive status from kafka-agent")
    die(message=message)


if __name__ == '__main__':
    _main()

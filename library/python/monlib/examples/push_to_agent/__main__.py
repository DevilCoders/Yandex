# Pushing directly to Solomon is highly unrecommended.
# If you need to push metrics, push it to Unified Agent
# and setup Solomon to pull metrics from agent.

# This example pushes to Unified Agent metrics_push input.

import time
import random
import os
import sys
import requests

from library.python.monlib.metric_registry import MetricRegistry, HistogramType
from library.python.monlib.encoder import dumps


def main():
    if "SOLOMON_TOKEN" not in os.environ:
        print("SOLOMON_TOKEN missing in env vars")
        sys.exit(1)

    if "UNIFIED_AGENT_HOST" not in os.environ:
        print("UNIFIED_AGENT_HOST missing in env vars")
        sys.exit(1)

    if "UNIFIED_AGENT_PORT" not in os.environ:
        print("UNIFIED_AGENT_PORT missing in env vars")
        sys.exit(1)

    solomon_token = os.environ["SOLOMON_TOKEN"]
    host = os.environ["UNIFIED_AGENT_HOST"]
    port = os.environ["UNIFIED_AGENT_PORT"]

    registry = MetricRegistry()

    job_time_hist = registry.histogram_rate(
        {'name': 'job_time'},
        HistogramType.Explicit, buckets=[10, 20, 50, 200, 500])

    for i in range(5):
        start = time.time()
        time.sleep(random.randint(50, 400) / 1000)
        job_time = time.time() - start
        print(f"Mock job took {job_time}s")
        job_time_hist.collect(job_time)

    data = dumps(registry, format='spack')
    headers={'Content-Type': 'application/x-solomon-spack', 'Authorization': f'OAuth {solomon_token}'}
    requests.post(f'http://{host}:{port}/write', data=data, headers=headers)


if __name__ == '__main__':
    main()

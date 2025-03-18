import os
import random
import time

import flask

from library.python.monlib.metric_registry import MetricRegistry, HistogramType
from library.python.monlib.encoder import dumps


CONTENT_TYPE_SPACK = 'application/x-solomon-spack'
CONTENT_TYPE_JSON = 'application/json'


def main():
    app = flask.Flask(__name__)

    registry = MetricRegistry()

    # we can cache metrics or create them/access existing ones like we do it in write_stats via registry
    response_times = registry.histogram_rate(
        {'path': 'ping', 'name': 'responseTimeMillis'},
        HistogramType.Explicit, buckets=[10, 20, 50, 200, 500])

    requests = registry.rate({'path': 'ping', 'name': 'requestRate'})
    uptime = registry.gauge({'name': 'serverUptimeSeconds'})

    start_time = time.time()

    @app.after_request
    def write_stats(resp):
        endpoint = flask.request.path.strip('/').replace('/', '_')

        if resp.status_code >= 200 and resp.status_code < 300:
            registry.rate({'path': endpoint, 'name': 'http.ok'}).inc()
        elif resp.status_code >= 400 and resp.status_code < 500:
            registry.rate({'path': endpoint, 'name': 'http.client_error'}).inc()
        elif resp.status_code >= 500 and resp.status_code < 600:
            registry.rate({'path': endpoint, 'name': 'http.server_error'}).inc()

        return resp

    # serves metrics in Solomon format
    @app.route('/metrics')
    def metrics():
        uptime.set(time.time() - start_time)

        # solomon fetcher will set the Accept header to application/x-solomon-spack,
        # which is more efficient than JSON
        if flask.request.headers.get('accept', None) == CONTENT_TYPE_SPACK:
            return flask.Response(dumps(registry), mimetype=CONTENT_TYPE_SPACK)

        # but it's a good idea to leave ability to visually inspect JSON
        return flask.Response(dumps(registry, format='json'), mimetype=CONTENT_TYPE_JSON)

    # does some stuff
    @app.route('/ping')
    def ping():
        req_start = time.time()
        time.sleep(random.random())

        requests.inc()
        response_times.collect((time.time() - req_start) * 1000)
        return 'pong'

    app.run(host='::', port=os.getenv('PORT', 1337))


if __name__ == '__main__':
    main()

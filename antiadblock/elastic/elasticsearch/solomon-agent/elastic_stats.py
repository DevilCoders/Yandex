import socket
import json
import re
import httplib


def get_hostname():
    HOSTNAME_TEMPL = r'(.*?\.)?(?P<fqdn>(?P<pod_id>\w+)\.(?P<dc>\w+)\.yp-c\.yandex\.net)'
    HOSTNAME_RE = re.compile(HOSTNAME_TEMPL)
    m = HOSTNAME_RE.match(socket.gethostname())
    return m.group('fqdn')


def get_http_json(query):
    conn = httplib.HTTPConnection(HTTP_ADDRESS)
    try:
        conn.request("GET", query)
        return json.loads(conn.getresponse().read().decode('utf8'))
    finally:
        conn.close()


PORT = '8890'
HOSTNAME = get_hostname()
HTTP_ADDRESS = '{}:{}'.format(HOSTNAME, PORT)

CLUSTER_HEALTH_API = '/_cluster/health?local=true'
NODES_API = '/_nodes/{}/stats/jvm,fs,http,thread_pool,indices'.format(HOSTNAME)


class ElasticModule:
    def __init__(self, logger, *_):
        self._logger = logger
        self._logger.info("New instance of ElasticModule")

    def get_cluster_health_metrics(self):
        data = get_http_json(CLUSTER_HEALTH_API)
        del data['cluster_name']
        data['status'] = 0 if data['status'] == 'green' else 1 if data['status'] == 'yellow' else 2
        return data

    def get_node_metrics(self, ts, consumer):
        data = get_http_json(NODES_API)['nodes'].items()[0][1]
        fs_total_json = data['fs']['total']
        disk_used_percent = 100 * (1 - (float(fs_total_json['available_in_bytes']) / fs_total_json['total_in_bytes']))
        consumer.gauge(dict(stats='diskUsedPercent'), ts, disk_used_percent)
        http_json = data['http']
        consumer.gauge(dict(stats='http_current_open'), ts, http_json['current_open'])
        jvm_mem_json = data['jvm']['mem']
        consumer.gauge(dict(stats='heapPercent'), ts, float(jvm_mem_json['heap_used_percent']))
        consumer.gauge(dict(stats='non_heap_used_gb'), ts, float(jvm_mem_json['non_heap_used_in_bytes']) / 2 ** 30)

        def consume_nested_metrics(metric):
            for sub_metric_name, sub_metrics in data[metric].items():
                for metric_name, metric_value in sub_metrics.items():
                    if isinstance(metric_value, int):
                        consumer.gauge(
                            {'stats': metric, 'sub_metric': str(sub_metric_name), 'metric': str(metric_name)}, ts,
                            int(metric_value))

        consume_nested_metrics('thread_pool')
        consume_nested_metrics('indices')

    def pull(self, ts, consumer):
        self.get_node_metrics(ts, consumer)
        for key, val in self.get_cluster_health_metrics().items():
            if type(val) == int or type(val) == float:
                consumer.gauge(dict(stats=str(key)), ts, val)

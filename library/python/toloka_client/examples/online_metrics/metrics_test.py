# pip install toloka-kit==0.1.5
import time
import datetime
import re
import toloka.client as toloka
from toloka.metrics import Balance, MetricCollector, AssignmentEventsInPool, AssignmentsInPool, bind_client
from toloka.client.analytics_request import SubmitedAssignmentsCountPoolAnalytics
from toloka.client._converter import structure

token = ''
assert token != ''
toloka_client = toloka.TolokaClient(token, 'PRODUCTION')

pool_id = ''
assert pool_id != ''

collector = MetricCollector(
    [
        Balance(),
        AssignmentEventsInPool(pool_id),
        AssignmentsInPool(pool_id),
    ]
)

bind_client(collector.metrics, toloka_client)

def send_metric_to_zabbix(metric_dict):
    print(f'\n\n{"-"*30}\nnew metrics:')
    for name, points in metric_dict.items():
        print(f'\n{name}')
        print(f'    {points}')

while True:
    metric_dict = collector.get_lines()
    send_metric_to_zabbix(metric_dict)
    time.sleep(10)

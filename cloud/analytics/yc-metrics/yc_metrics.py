import os
import glob
import json
from yql.api.v1.client import YqlClient



final_query = """
use hahn;

PRAGMA yt.Pool = 'cloud_analytics_pool';
PRAGMA yt.QueryCacheMode="disable";

PRAGMA Library('tables.sql');
PRAGMA Library('time.sql');
PRAGMA Library('time_series.sql');

"""

curr_path = '/'.join(os.path.realpath(__file__).split('/')[:-1])


metric_queries_files = sorted(glob.glob(curr_path + '/metrics/*/*.sql', recursive=True))
metric_names = [m.split('/')[-1] for m in metric_queries_files]

libs_files = sorted(glob.glob(curr_path + '/lib/**/*.sql', recursive=True))
libs_names = [m.split('/')[-1] for m in libs_files]

for q in metric_names + libs_names:
    final_query += """
PRAGMA Library('{0}');
IMPORT {1} SYMBOLS ${1};
    """.format(q, q.split('.')[0])

final_query += """


INSERT INTO `//home/cloud_analytics/yc-metrics/yc-metrics` WITH TRUNCATE
"""


def select_all_query(q):
    return """
SELECT * FROM
${0}()
    """.format(q.split('.')[0])

final_query += '\nUNION ALL\n'.join([select_all_query(q) for q in metric_names])

client = YqlClient()
request = client.query(final_query, syntax_version=1, title='YQL YC METRICS')

request.attach_file(curr_path + '/clan_tools/src/clan_tools/utils/yql/time.sql', 'time.sql')
request.attach_file(curr_path + '/clan_tools/src/clan_tools/utils/yql/tables.sql', 'tables.sql')
request.attach_file(curr_path + '/time_series.sql', 'time_series.sql')
for f,n in zip(metric_queries_files + libs_files, metric_names + libs_names):
    request.attach_file(f, n)
request.run()

print('\nStarted query: ' + request.share_url)

results = request.get_results()


with open(curr_path + '/output.json', 'w') as f:
        json.dump({
            "status": results.status.lower(),
            "share_url": request.share_url
        }, f)


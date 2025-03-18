# encoding=utf8
# Исходник бинаря для подсчета статистики АВАПСА (показы рекламы по типам шаблонов) на Морде
import os
import argparse
from datetime import timedelta, datetime as dt

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yql.api.v1.client import YqlClient
import statface_client

from antiadblock.tasks.tools.logger import create_logger
import antiadblock.tasks.tools.common_configs as configs


logger = create_logger('morda_awaps_by_templates')
YT_CLUSTER = 'hahn'
MORDA_TEMPLATES_REPORT = 'AntiAdblock/morda_by_templates'
templates = NativeEnvironment(loader=DictLoader({"awaps_templates": bytes.decode(resource.find("awaps_templates"))}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for calculating morda awaps shows (grouped by templates) and uploading results to Stat')
    parser.add_argument('--range', nargs=2, metavar='xxxx-xx-xx', help='Range of log files in yql request (Y-m-d format)')
    args = parser.parse_args()

    yt_token = os.getenv('YT_TOKEN')
    assert yt_token is not None
    stat_token = os.getenv('STAT_TOKEN')
    assert stat_token is not None
    table_name_fmt = configs.YT_TABLES_DAILY_FMT

    if args.range:
        start, end = map(lambda t: dt.strptime(t, table_name_fmt), args.range)
        assert start <= end
    else:
        end = dt.now().replace(hour=0, minute=0, second=0, microsecond=0)
        start = end - timedelta(days=2)

    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)
    stat_client = statface_client.StatfaceClient(host=statface_client.STATFACE_PRODUCTION, oauth_token=stat_token)
    report = stat_client.get_report(MORDA_TEMPLATES_REPORT)

    query = templates.get_template('awaps_templates').render(file=__file__, start=start, end=end, table_name_fmt=table_name_fmt)
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')
    yql_response = yql_request.run()
    logger.info('Waiting morda awaps shows (grouped by templates) yql request')
    df = yql_response.full_dataframe
    df = df[df.fielddate.map(lambda t: dt.strptime(t, configs.STAT_FIELDDATE_I_FMT)) >= start]
    logger.info(f'Results dataframe with shape {df.shape}:\n{df.head()}\n\n')

    logger.info("Uploading data to stat report")
    logger.info('Trying to upload results to Stat report (scale=h):')
    upload_result = report.upload_data(scale=configs.Scales.hour.value, data=map(lambda row: dict(row[1]), df.iterrows()))
    logger.info(upload_result)

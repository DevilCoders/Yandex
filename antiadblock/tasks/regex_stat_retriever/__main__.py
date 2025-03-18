import argparse
import os

from library.python import resource
from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('regex_stat_retriever')

YT_CLUSTER = 'hahn'
templates = NativeEnvironment(loader=DictLoader({
    "regex_stat_retriever_query": bytes.decode(resource.find("regex_stat_retriever_query"))
}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--result-dir', type=str, default='./stats/')
    parser.add_argument('--yt-logs-path', type=str, default='//home/logfeller/logs/antiadb-cryprox-log/stream/5min')
    parser.add_argument('--tables-to-use', type=int, default=6)
    args = parser.parse_args()

    yql_token = os.getenv('YQL_TOKEN')
    assert (yql_token is not None and len(yql_token) > 0)

    os.makedirs(args.result_dir, exist_ok=True)

    yql_client = YqlClient(token=yql_token, db=YT_CLUSTER)

    query = templates.get_template("regex_stat_retriever_query").render(yt_logs_path=args.yt_logs_path, tables_to_use=args.tables_to_use)
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info('Running yql query...')
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')

    def echo_urls(yql_req):
        logger.info('Operation URL: {}'.format(yql_req.share_url))
    yql_response = yql_request.run(pre_start_callback=echo_urls)

    df = yql_response.full_dataframe
    logger.info('Query has completed')
    logger.info('Writing results...')
    logger.info(df.head())

    logfiles = dict()

    def get_logfile(service_id):
        if service_id in logfiles:
            return logfiles[service_id]
        logger.info(service_id)
        logfile = open(os.path.join(args.result_dir, service_id), 'w')
        logfiles[service_id] = logfile
        return logfile

    for _, row in df.iterrows():
        service_id, regex, urls = row
        urls.sort()
        logfile = get_logfile(service_id)
        logfile.write('\n'.join(['=' * 100, regex, '-' * 100] + urls + ['='*100, '', '']))

    for k, logfile in logfiles.items():
        logfile.close()

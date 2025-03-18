import argparse
import os

from library.python import resource
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('popup_blocked_domains_extractor_checker')

YT_CLUSTER = 'hahn'
checker_sql = bytes.decode(resource.find('popup_blocked_domains_extractor_checker'))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='This tool shows the rules that we didn\'t parse domain from'
                                                 ' but which might possibly have it')
    parser.add_argument('--result-path',
                        type=str,
                        default='unparsed_popup_blocked_domains.tsv')
    args = parser.parse_args()

    yt_token = os.getenv('YT_TOKEN')
    assert (yt_token is not None)

    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)

    query = checker_sql
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info('Running yql query...')
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')

    def echo_urls(yql_req):
        logger.info('Operation URL: {}'.format(yql_req.share_url))
    yql_response = yql_request.run(pre_start_callback=echo_urls)

    df = yql_response.full_dataframe
    logger.info('Query has completed')
    logger.info('Writing dataframe into {}'.format(args.result_path))

    df.to_csv(args.result_path, index=False, encoding='utf-8', sep='\t')

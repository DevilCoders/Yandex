import argparse
import os

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.logger import create_logger


logger = create_logger('popup_blocked_domains')

YT_CLUSTER = 'hahn'
templates = NativeEnvironment(loader=DictLoader({
    "extractor": bytes.decode(resource.find("popup_blocked_domains_extractor")),
}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='This tool extracts popup-blocked domains from general_rules')
    parser.add_argument('--upload-to-yt',
                        action='store_true',
                        help='upload the result into yt table instead of saving it locally')
    parser.add_argument('--yt-path',
                        default='home/antiadb/sonar/popup_blocked_domains')
    parser.add_argument('--result-path',
                        type=str,
                        default='popup_blocked_domains.tsv')
    args = parser.parse_args()

    if args.upload_to_yt:
        logger.info('Will upload extracted domains into {}.`{}`'.format(YT_CLUSTER, args.yt_path))
    else:
        logger.info('Will save extracted domains into {}'.format(args.result_path))

    yt_token = os.getenv('YT_TOKEN')
    assert(yt_token is not None)

    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)

    query = templates.get_template('extractor').render(
        upload_to_yt=args.upload_to_yt,
        yt_path=args.yt_path
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info('Running yql query...')
    yql_request = yql_client.query(query, syntax_version=1, title=f'{__file__.split("/", 1)[1]} YQL')

    def echo_urls(yql_req):
        logger.info('Operation URL: {}'.format(yql_req.share_url))
    yql_response = yql_request.run(pre_start_callback=echo_urls)

    df = yql_response.full_dataframe
    logger.info('Query has completed')

    if not args.upload_to_yt:
        df.to_csv(args.result_path, index=False, encoding='utf-8', sep='\t')

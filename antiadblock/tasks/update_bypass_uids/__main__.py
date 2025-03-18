# encoding=utf8
# Скрит, обновляющий SB ресурс ANTIADBLOCK_BYPASS_UIDS - yadnexuid-ы пользователей без блокировщиков
import argparse
import os
from os import getenv
import tempfile
import subprocess
from datetime import datetime, timedelta

from jinja2 import DictLoader
from jinja2.nativetypes import NativeEnvironment

from library.python import resource
import yt.wrapper as yt
from yql.api.v1.client import YqlClient

from antiadblock.tasks.tools.common_configs import UIDS_STATE_TABLE_PATH, NO_ADBLOCK_DOMAINS_PATH, NO_ADBLOCK_UIDS_PATH, YT_TABLES_DAILY_FMT
from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('update_bypass_uids_logger')
templates = NativeEnvironment(loader=DictLoader({
    "domains": bytes.decode(resource.find("no_adblock_domains.sql")),
    "uids": bytes.decode(resource.find("no_adblock_uids.sql")),
}))

YT_CLUSTER = 'hahn'
YAUID_COOKIE_MAX_LENGTH = 20
BYPASS_UIDS_RESOURCE_NAME = 'ANTIADBLOCK_BYPASS_UIDS'

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Tool for uploading new ANTIADBLOCK_BYPASS_UIDS SB resource')
    parser.add_argument('--update_domains',
                        action='store_true',
                        help='Update list of domains without any antiadblock solutions')

    args = parser.parse_args()
    yt_token = getenv('YT_TOKEN')
    assert yt_token is not None
    sb_token = getenv('SB_TOKEN')
    assert sb_token is not None
    update_domains = args.update_domains or getenv('UPDATE_DOMAINS')

    yt_client = yt.YtClient(token=yt_token, proxy=YT_CLUSTER)
    yql_client = YqlClient(token=yt_token, db=YT_CLUSTER)

    end = datetime.now()
    start = end - timedelta(weeks=1)

    if update_domains:
        query = templates.get_template("domains").render(
            file=__file__, start=start, end=end, table_name_fmt=YT_TABLES_DAILY_FMT,
            uds_state_table=UIDS_STATE_TABLE_PATH,
            no_adblock_domains_path=NO_ADBLOCK_DOMAINS_PATH,
        )
        query = 'PRAGMA yt.Pool="antiadb";\n' + query
        logger.info(f'YQL request to update no Adblock domains table, query:\n{query}\n\n')
        yql_result = yql_client.query(query, syntax_version=1, title=f'DOMAINS {__file__.split("/", 1)[1]} YQL').run().get_results()
        logger.info(yql_result)

    no_adblock_domains_table = NO_ADBLOCK_DOMAINS_PATH + '/' + sorted(yt_client.list(NO_ADBLOCK_DOMAINS_PATH))[-1]
    query = templates.get_template("uids").render(
        file=__file__, start=start, end=end, table_name_fmt=YT_TABLES_DAILY_FMT,
        no_adblock_domains_table=no_adblock_domains_table,
        no_adblock_domains_path=NO_ADBLOCK_UIDS_PATH,
    )
    query = 'PRAGMA yt.Pool="antiadb";\n' + query
    logger.info(f'YQL request to update no Adblock uids table, query:\n{query}\n\n')
    yql_result = yql_client.query(query, syntax_version=1, title=f'USERS {__file__.split("/", 1)[1]} YQL').run().get_results()
    logger.info(yql_result)

    tmp_dir = tempfile.mkdtemp()
    logger.info(f'Created tmp dir {tmp_dir}')

    bypass_uids_file = os.path.join(tmp_dir, 'bupass-uids')
    logger.info('Downloading no adblock uids')
    no_adblock_uids_table = NO_ADBLOCK_UIDS_PATH + '/' + sorted(yt_client.list(NO_ADBLOCK_UIDS_PATH))[-1]
    with open(bypass_uids_file, 'wb') as f:
        for row in yt_client.read_table(no_adblock_uids_table, raw=False, format=yt.JsonFormat(attributes={"encode_utf8": False})):
            f.write(str(row['uniqid']).zfill(YAUID_COOKIE_MAX_LENGTH).encode('ascii'))
    logger.info('No adblock uids downloaded to {}'.format(bypass_uids_file))

    logger.info('Get Ya Tool')
    subprocess.call(['svn', 'export', 'svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/ya', tmp_dir])

    logger.info('uploading no adblock uids as SB resource')
    subprocess.call([os.path.join(tmp_dir, 'ya'), 'upload',
                     '--type', BYPASS_UIDS_RESOURCE_NAME,
                     '--ttl', '14',
                     '--owner', 'ANTIADBLOCK',
                     '--description', 'No Adblock uids',
                     '--token', sb_token,
                     bypass_uids_file])

    logger.info('Removing temp directory')
    try:
        for tmp_file in os.listdir(tmp_dir):
            os.remove(os.path.join(tmp_dir, tmp_file))
        os.rmdir(tmp_dir)
    except OSError:
        logger.error(f"Deletion of the directory {tmp_dir} failed")
    else:
        logger.error(f"Successfully deleted the directory {tmp_dir}")

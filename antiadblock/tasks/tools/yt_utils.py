import os
import time
import json
from datetime import timedelta, datetime as dt

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.common_configs import YT_TABLES_DAILY_FMT


logger = create_logger('yt_utils_logger')


def mount_dynamic_table(yt_client, table, timeout=60):
    SLEEP_TIME = 0.1

    if yt_client.get("{0}/@tablets/0/state".format(table)) == 'mounted':
        # already mounted
        return True

    yt_client.mount_table(table)
    while timeout > 0:
        if yt_client.get("{0}/@tablets/0/state".format(table)) != 'mounted':
            time.sleep(SLEEP_TIME)
            timeout -= SLEEP_TIME
        else:
            return True
    return False


def ensure_yt_dynamic_table(yt_client, table, schema):
    logger.info('Checking if table {} exists'.format(table))
    if not yt_client.exists(table):
        logger.info('Creating table PATH={}'.format(table))
        yt_client.create(
            type='table',
            path=table,
            attributes=dict(schema=schema, dynamic=True)
        )
    mounted = False
    for _ in range(3):
        mounted = mount_dynamic_table(yt_client, table)
        if mounted:
            break
    if not mounted:
        raise Exception("Error while mounting YT table")


def remove_old_tables(yt_client, yt_dir, days=7):
    for table_name in filter(lambda x: x < (dt.now() - timedelta(days=days)).strftime(YT_TABLES_DAILY_FMT), yt_client.list(yt_dir)):
        yt_client.remove(os.path.join(yt_dir, table_name))


def get_available_yt_cluster(curr_yt_cluster, available_yt_cluster):
    if available_yt_cluster is None:
        return curr_yt_cluster

    available_yt_cluster = json.loads(available_yt_cluster)
    if not available_yt_cluster:
        raise Exception("All YT cluster is not available")
    return curr_yt_cluster if curr_yt_cluster in available_yt_cluster else available_yt_cluster[0]

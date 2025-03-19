import logging

from datetime import timedelta
from datetime import datetime

import yt.wrapper as yt

from cloud.blockstore.tools.analytics.common import add_expiration_time

NBS_CLIENT_SYSLOG_IDENTIFIER = "NBS_CLIENT"
NBS_SERVER_SYSLOG_IDENTIFIER = "NBS_SERVER"
BLOCKSTORE_TRACE_COMPONENT = "BLOCKSTORE_TRACE"
COMPUTE_NODE_UNIT = "yc-compute-node.service"


NBS_LOGS_SCHEMA = {
    'schema': [
        {
            'name': 'component',
            'type': 'string',
        },
        {
            'name': 'host',
            'type': 'string',
        },
        {
            'name': 'message',
            'type': 'string',
        },
        {
            'name': 'priority',
            'type': 'string',
        },
        {
            'name': 'syslog_identifier',
            'type': 'string',
        },
        {
            'name': 'timestamp',
            'type': 'string',
        },

    ]
}


def filter_nbs_logs(src, dst, proxy, expiration_time=None, pool=None, logger=logging.getLogger(), is_ua_log=False):

    def mapper_ua(input_row):
        timestamp = datetime.fromtimestamp(int(input_row['TIMESTAMP']) // 1000000)
        timestamp = timestamp + timedelta(microseconds=(int(input_row['TIMESTAMP']) % 1000000))

        output_row = {
            'message': input_row['MESSAGE'],
            'component': input_row['COMPONENT'],
            'priority': input_row['PRIORITY'],
            'host': input_row['HOSTNAME'],
            'timestamp': timestamp.isoformat(),
            'syslog_identifier': input_row["IDENTIFIER"]
        }

        if output_row['syslog_identifier'] not in [NBS_CLIENT_SYSLOG_IDENTIFIER, NBS_SERVER_SYSLOG_IDENTIFIER]:
            return

        if output_row['component'] == BLOCKSTORE_TRACE_COMPONENT:
            return

        yield output_row

    def mapper(input_row):
        syslog_identifier = input_row["SYSLOG_IDENTIFIER"]

        if syslog_identifier not in [NBS_CLIENT_SYSLOG_IDENTIFIER, NBS_SERVER_SYSLOG_IDENTIFIER]:
            return

        message = input_row['MESSAGE']
        sp_message = message.split()
        component = sp_message[1][1:]

        if component == BLOCKSTORE_TRACE_COMPONENT:
            return

        priority = sp_message[2][:-1]
        hostname = input_row['HOSTNAME']
        timestamp = input_row['iso_eventtime']

        output_row = {
            'message': message,
            'component': component,
            'priority': priority,
            'host': hostname,
            'timestamp': timestamp,
            'syslog_identifier': syslog_identifier
        }
        yield output_row

    logger = logger.getChild('filter-nbs-logs')
    yt.config['proxy']['url'] = proxy

    if not yt.exists(src):
        logger.info('No such source table [src={}]'.format(src))
        return

    if yt.exists(dst):
        logger.info('Dst table already exists [dst={}]'.format(dst))
        return

    logger.info('Filtering NBS logs [src={}], [dst={}]'.format(src, dst))
    yt.create('table', dst, attributes=NBS_LOGS_SCHEMA)
    yt.set(yt.ypath_join(dst, '@optimize_for'), 'scan')
    add_expiration_time(dst, expiration_time)
    yt.run_map(mapper_ua if is_ua_log else mapper, src, dst, spec={'pool': pool, 'max_failed_job_count': 100})
    yt.run_sort(dst, sort_by='timestamp')
    logger.info("NBS logs have been filtered [src={}], [dst={}]".format(src, dst))


def filter_nbs_traces(src, dst, proxy, expiration_time=None, pool=None, logger=logging.getLogger(), is_ua_log=False):

    def mapper_ua(input_row):
        timestamp = datetime.fromtimestamp(int(input_row['TIMESTAMP']) // 1000000)
        timestamp = timestamp + timedelta(microseconds=(int(input_row['TIMESTAMP']) % 1000000))

        output_row = {
            'message': input_row['MESSAGE'],
            'component': input_row['COMPONENT'],
            'priority': input_row['PRIORITY'],
            'host': input_row['HOSTNAME'],
            'timestamp': timestamp.isoformat(),
            'syslog_identifier': input_row["IDENTIFIER"]
        }

        if output_row['syslog_identifier'] != NBS_SERVER_SYSLOG_IDENTIFIER:
            return

        if output_row['component'] != BLOCKSTORE_TRACE_COMPONENT:
            return

        yield output_row

    def mapper(input_row):
        syslog_identifier = input_row["SYSLOG_IDENTIFIER"]

        if syslog_identifier != NBS_SERVER_SYSLOG_IDENTIFIER:
            return

        message = input_row['MESSAGE']
        sp_message = message.split()
        component = sp_message[1][1:]

        if component != BLOCKSTORE_TRACE_COMPONENT:
            return

        priority = sp_message[2][:-1]
        hostname = input_row['HOSTNAME']
        timestamp = input_row['iso_eventtime']

        output_row = {
            'message': message,
            'component': component,
            'priority': priority,
            'host': hostname,
            'timestamp': timestamp,
            'syslog_identifier': syslog_identifier
        }
        yield output_row

    logger = logger.getChild('filter-nbs-traces')
    yt.config['proxy']['url'] = proxy

    if not yt.exists(src):
        logger.info('No such source table [src={}]'.format(src))
        return

    if yt.exists(dst):
        logger.info('Dst table already exists [dst={}]'.format(dst))
        return

    logger.info('Filtering NBS traces [src={}], [dst={}]'.format(src, dst))
    yt.create('table', dst, attributes=NBS_LOGS_SCHEMA)
    yt.set(yt.ypath_join(dst, '@optimize_for'), 'scan')
    add_expiration_time(dst, expiration_time)
    yt.run_map(mapper_ua if is_ua_log else mapper, src, dst, spec={'pool': pool, 'max_failed_job_count': 100})
    yt.run_sort(dst, sort_by='timestamp')
    logger.info("NBS traces have been filtered [src={}], [dst={}]".format(src, dst))


def filter_qemu_logs(src, dst, proxy, expiration_time=None, pool=None, logger=logging.getLogger(), is_ua_log=False):

    def mapper_ua(input_row):
        timestamp = datetime.fromtimestamp(int(input_row['TIMESTAMP']) // 1000000)
        timestamp = timestamp + timedelta(microseconds=(int(input_row['TIMESTAMP']) % 1000000))

        output_row = {
            'message': input_row['MESSAGE'],
            'component': input_row['COMPONENT'],
            'priority': input_row['PRIORITY'],
            'host': input_row['HOSTNAME'],
            'timestamp': timestamp.isoformat(),
            'syslog_identifier': input_row["IDENTIFIER"]
        }

        if output_row['syslog_identifier'] != NBS_CLIENT_SYSLOG_IDENTIFIER:
            return

        if output_row['component'] not in ['BLOCKSTORE_CLIENT', 'BLOCKSTORE_PLUGIN']:
            return

        yield output_row

    def mapper(input_row):
        unit = input_row['UNIT']
        message = input_row['MESSAGE']

        if not (unit == COMPUTE_NODE_UNIT and message.find('QEMU output') != -1):
            return

        hostname = input_row['HOSTNAME']
        timestamp = input_row['iso_eventtime']
        messages = message.splitlines()

        for message in messages:
            if not (message.find('BLOCKSTORE_CLIENT') != -1 or message.find('BLOCKSTORE_PLUGIN') != -1):
                continue
            sp_message = message.split()
            if len(sp_message) < 4:
                continue
            offset = 0
            if sp_message[0] == "qemu-system-x86_64:":
                offset = 1
            component = sp_message[1 + offset][1:]
            priority = sp_message[2 + offset][:-1]

            output_row = {
                'message': message,
                'component': component,
                'priority': priority,
                'host': hostname,
                'timestamp': timestamp
            }

            yield output_row

    logger = logger.getChild('filter-qemu-logs')
    yt.config['proxy']['url'] = proxy

    if not yt.exists(src):
        logger.info('No such source table [src={}]'.format(src))
        return

    if yt.exists(dst):
        logger.info('Dst table already exists [dst={}]'.format(dst))
        return

    logger.info('Filtering QEMU logs [src={}], [dst={}]'.format(src, dst))
    yt.create('table', dst, attributes=NBS_LOGS_SCHEMA)
    yt.set(yt.ypath_join(dst, '@optimize_for'), 'scan')
    add_expiration_time(dst, expiration_time)
    yt.run_map(mapper_ua if is_ua_log else mapper, src, dst, spec={'pool': pool, 'max_failed_job_count': 100})
    yt.run_sort(dst, sort_by='timestamp')
    logger.info("QEMU logs have been filtered [src={}], [dst={}]".format(src, dst))

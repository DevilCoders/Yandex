#!/usr/bin/env python3
"""
Code that implements reading job launcher's output from s3
"""

import copy
import logging
import re
import time
import webbrowser
from datetime import datetime, timedelta, timezone
from io import StringIO
from sys import stdout
from typing import Dict

from botocore.exceptions import ClientError

from tests.helpers import internal_api, metadb, s3

from .utils import FakeContext, env_stage, ssh


class NewJobWatcher:
    """
    Wait for new job and attach to its launcher's output
    """

    def __init__(self, context):
        self.context = context

    def run(self):
        self.open_socks_tunnel()
        logging.info('Waiting for new jobs')
        while True:
            job = metadb.running_job(self.context)
            if job:
                job_id = job['job_id']
                logging.info(f'Found job {job_id}. Waiting for its output...')
                JobOutput(self.context, job_id, job['cid']).watch('socks_tunnel_port' in self.context.conf)
                logging.info('Waiting for new jobs')
            else:
                time.sleep(1)

    def open_socks_tunnel(self):
        port = self.context.conf.get('socks_tunnel_port')
        if port:
            gateway_hostname = self.context.conf['compute_driver']['fqdn']
            code, out, err = ssh(gateway_hostname, ['lsof', f'-i:{port}'])
            if code != 0:
                internal_api.ensure_cluster_is_loaded_into_context(self.context)
                master_node = next(host for host in self.context.hosts if host['role'] == 'MASTERNODE')
                master_node_hostname = master_node['name']
                ssh_key_path = self.context.conf['compute_driver']['ssh_keys']['dataplane']['private']

                ssh(
                    gateway_hostname,
                    [
                        f'nohup ssh -i {ssh_key_path} -o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null"'
                        f' -fC2qTN -D :{port} {master_node_hostname}'
                        f' >/dev/null 2>/dev/null </dev/null &',
                    ],
                )
                logging.info("Opened SOCKS tunnel")
            else:
                logging.info("SOCKS tunnel is already open")


class JobOutput:
    """
    Implements access to launcher's output of specific job
    """

    def __init__(self, context, job_id, cid, bucket=None):
        self.context = context
        self.job_id = job_id
        self.cid = cid
        self.chunk_size = 4096
        self.chunk_index = 0
        self.chunk_printed_bytes = 0

        self.s3config = copy.deepcopy(context.conf['s3'])
        if bucket:
            self.s3config['bucket_name'] = bucket

        self.not_processed_output = ''
        self.open_tracking_url = False
        self.tracking_url = None

    def watch(self, open_tracking_url=True):
        """
        Monitors launcher's output and prints it to stdout until job is terminated
        """
        self.open_tracking_url = open_tracking_url
        while True:
            self.read_and_print_new_output(stdout)
            job = metadb.get_job(self.context, self.job_id)
            terminated_at = job['end_ts'] if job else None
            # give some time for launcher's output to sync to s3
            if terminated_at and datetime.now(timezone.utc) - terminated_at > timedelta(seconds=10):
                break
            time.sleep(1)

    def download(self):
        """
        Returns whole launcher's output as a string
        """
        output = StringIO()
        self.read_and_print_new_output(output)
        return output.getvalue()

    def read_and_print_new_output(self, dest):
        chunk_content = self.read_chunk_content()
        to_print = chunk_content[self.chunk_printed_bytes :]
        if len(to_print) == 0:
            return
        dest.write(to_print)
        dest.flush()
        if self.open_tracking_url:
            self.scan_output(to_print)
        self.chunk_printed_bytes = len(chunk_content)
        if self.chunk_printed_bytes >= self.chunk_size:
            self.chunk_index += 1
            self.chunk_printed_bytes = 0
            self.read_and_print_new_output(dest)

    def read_chunk_content(self):
        idx = '%09d' % self.chunk_index
        object_name = f'dataproc/clusters/{self.cid}/jobs/{self.job_id}/driveroutput.{idx}'
        try:
            return s3.download_object(self.s3config, self.s3config['bucket_name'], object_name)
        except ClientError as ex:
            if ex.response['Error']['Code'] == 'NoSuchKey':
                return ''
            else:
                raise

    def scan_output(self, new_output):
        self.not_processed_output += new_output
        lines = self.not_processed_output.split('\n')
        lines_to_scan = lines
        # may not process last line if it is not fully downloaded (not ends with \n)
        if len(lines[-1]) == 0:
            lines_to_scan = lines_to_scan[:-1]
        self.not_processed_output = lines[-1]
        for line in lines_to_scan:
            self.process_line(line)

    def process_line(self, line):
        match = re.match(r'\s*tracking URL: (.*)', line) or re.match(r'.*The url to track the job: (.*)', line)
        if match and not self.tracking_url:
            self.tracking_url = match[1]
            webbrowser.open(self.tracking_url)


@env_stage('watch_job_output', fail=True)
def watch(state: Dict, conf: Dict, **_) -> None:
    """
    Wait for new job and attach to its launcher's output
    """
    try:
        context = FakeContext(state, conf)
        NewJobWatcher(context).run()
    except KeyboardInterrupt:
        logging.info("Buy-buy!")

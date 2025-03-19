#!/usr/bin/env python3
"""
Prepare jenkins for gracefull shutdown
"""

import time
import urllib.parse

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


class RestartPrepare:
    """
    Prepare jenkins for gracefull shutdown
    """

    def __init__(self, max_wait_time, url='http://localhost:8080/'):
        self.max_wait_time = max_wait_time
        self.session = requests.Session()
        retries = Retry(total=5, backoff_factor=1, status_forcelist=[500, 502, 503, 504])
        adapter = HTTPAdapter(max_retries=retries, pool_connections=1, pool_maxsize=1)
        self.session.mount(url, adapter)
        self.base_url = url

    def get_crumb(self):
        """
        Get crumb for post requests
        """
        res = self.session.get(
            urllib.parse.urljoin(self.base_url, '/crumbIssuer/api/json'),
            headers={'X-Forwarded-User': 'admin'},
        )
        res.raise_for_status()
        data = res.json()
        return {
            'X-Forwarded-User': 'admin',
            data['crumbRequestField']: data['crumb'],
        }

    def get_num_running_jobs(self):
        """
        Get number of running jobs on all nodes
        """
        res = self.session.get(
            urllib.parse.urljoin(self.base_url, '/computer/api/json'),
            params={
                'depth': 2,
                'tree': 'computer[executors[currentExecutable[building,url]],'
                + 'oneOffExecutors[currentExecutable[building,url]]]',
                'xpath': '//currentExecutable[building="true"]/url',
                'wrapper': 'builds',
            },
        )
        res.raise_for_status()
        parsed = res.json()
        running = 0
        for node in parsed.get('computer', []):
            for executor in node.get('executors', []):
                job = executor.get('currentExecutable')
                if job is not None:
                    running += 1

        return running

    def set_prepare_to_shutdown(self):
        """
        Call prepare to shutdown api
        """
        res = self.session.post(
            urllib.parse.urljoin(self.base_url, '/quietDown'),
            headers=self.get_crumb(),
        )
        res.raise_for_status()

    def cancel_prepare_to_shutdown(self):
        """
        Cancel prepared shutdown
        """
        res = self.session.post(
            urllib.parse.urljoin(self.base_url, '/cancelQuietDown'),
            headers=self.get_crumb(),
        )
        res.raise_for_status()

    def prepare(self):
        """
        Wait for no running jobs and run prepare to shutdown
        """
        deadline = time.time() + self.max_wait_time

        was_prepared = False

        while time.time() < deadline:
            num_jobs = self.get_num_running_jobs()
            if num_jobs == 0:
                if not was_prepared:
                    self.set_prepare_to_shutdown()
                    was_prepared = True
                    time.sleep(10)  # Max quiet period
                else:
                    return
            elif was_prepared:
                self.cancel_prepare_to_shutdown()
                was_prepared = False

            time.sleep(5)

        raise RuntimeError(f'Timeout ({self.max_wait_time} seconds) exceeded')


def _main():
    prep = RestartPrepare(24 * 3600)
    prep.prepare()


if __name__ == '__main__':
    _main()

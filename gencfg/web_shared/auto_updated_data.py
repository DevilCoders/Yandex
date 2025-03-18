"""Data, which is updated automatically in background"""


import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import threading
import time

import utils.common.manipulate_hbf


class THbfGroupsUpdater(object):
    """Object, representing update of something"""
    def __init__(self, data_func, timeout):
        self.data = {}

        self.func = data_func
        self.timeout = timeout
        self.last_run_time = 0
        self.last_modified = 0

        self.updater_thread = threading.Thread(target=self.run)
        self.updater_thread.daemon = True
        self.finished = False

        self.updater_thread.start()

    def update_data(self):
        self.last_run_time = int(time.time())

        response = utils.common.manipulate_hbf.get_hbf_group_list(if_modified_since=self.last_modified)
        if response is not None:
            self.data = response
            self.last_modified = int(time.time())

    def run(self):
        while not self.finished:
            if time.time() < self.last_run_time + self.timeout:
                time.sleep(1)
                continue

            self.update_data()


HBF_GROUPS = THbfGroupsUpdater(utils.common.manipulate_hbf.get_hbf_group_list, 20)

import datetime
import importlib
import os

from .wrapper import Wrapper

from sandbox.sdk2 import Vault


def get_vault_data(secret):
    return str(Vault.data(secret.owner, secret.name))


class Task(Wrapper):

    def __init__(self, task, logger):
        super(Task, self).__init__(task)

        self.logger = logger.getChild('main')

        # must set YT_TOKEN environ before import
        os.environ["YT_TOKEN"] = get_vault_data(self.yt_oauth)
        self.module = importlib.import_module("cloud.blockstore.tools.analytics.filter-yt-logs.lib")

    def run(self):
        d = datetime.datetime.now()
        yesterday = (d - datetime.timedelta(days=1)).strftime("%Y-%m-%d")
        hours = [(d - datetime.timedelta(hours=i)).strftime("%Y-%m-%dT%H:00:00") for i in range(1, 5)]
        expiration_time = (d + datetime.timedelta(days=2)).isoformat()

        for folder_prefix in ["nbs-prod"]:
            self._filter_log(folder_prefix, "1d", yesterday, None)

            for hour in hours:
                self._filter_log(folder_prefix, "1h", hour, expiration_time)

        for folder_prefix in ["nbs-preprod", "nbs-testing", "nbs-stable-lab"]:
            self._filter_log(folder_prefix, "1d", yesterday, None, True)

            for hour in hours:
                self._filter_log(folder_prefix, "1h", hour, expiration_time, True)

    def _filter_log(self, folder_prefix, subfolder, date_str, expiration_time, is_ua_log=False):
        self.module.filter_nbs_logs(
            "//logs/%s-log/%s/%s" % (folder_prefix, subfolder, date_str),
            "//home/cloud-nbs/%s/logs/%s/%s" % (self._normalize_folder_prefix(folder_prefix), subfolder, date_str),
            "hahn",
            expiration_time,
            self.yt_pool,
            self.logger,
            is_ua_log
        )
        self.module.filter_nbs_traces(
            "//logs/%s-log/%s/%s" % (folder_prefix, subfolder, date_str),
            "//home/cloud-nbs/%s/traces/%s/%s" % (self._normalize_folder_prefix(folder_prefix), subfolder, date_str),
            "hahn",
            expiration_time,
            self.yt_pool,
            self.logger,
            is_ua_log
        )
        self.module.filter_qemu_logs(
            "//logs/%s-log/%s/%s" % (folder_prefix, subfolder, date_str),
            "//home/cloud-nbs/%s/qemu-logs/%s/%s" % (self._normalize_folder_prefix(folder_prefix), subfolder, date_str),
            "hahn",
            expiration_time,
            self.yt_pool,
            self.logger,
            is_ua_log
        )

    @staticmethod
    def _normalize_folder_prefix(folder_prefix):
        _map = {
            'nbs-prod': 'yandexcloud-prod',
            'nbs-preprod': 'yandexcloud-preprod',
            'nbs-testing': 'yandexcloud-testing',
            'nbs-stable-lab': 'yandexcloud-stable-lab'
        }
        return _map[folder_prefix]

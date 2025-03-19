import datetime
import importlib
import os

from .wrapper import Wrapper

from sandbox.projects.cloud.blockstore.resources import NbsPerformanceReport
from sandbox.sdk2 import ResourceData
from sandbox.sdk2 import Vault


def get_vault_data(secret):
    return str(Vault.data(secret.owner, secret.name))


class Task(Wrapper):

    def __init__(self, task, logger):
        super(Task, self).__init__(task)

        self.logger = logger.getChild('main')

        # must set YT_TOKEN environ before import
        os.environ["YT_TOKEN"] = get_vault_data(self.yt_oauth)
        self.lib = importlib.import_module("cloud.blockstore.tools.analytics.find-perf-bottlenecks.lib")
        self.ytlib = importlib.import_module("cloud.blockstore.tools.analytics.find-perf-bottlenecks.ytlib")

    def run(self):
        d = datetime.datetime.now()
        yesterday = (d - datetime.timedelta(days=1)).strftime("%Y-%m-%d")
        hours = [(d - datetime.timedelta(hours=i)).strftime("%Y-%m-%dT%H:00:00") for i in range(1, 5)]
        expiration_time = (d + datetime.timedelta(days=2)).isoformat()

        for folder_prefix in ["yandexcloud-preprod", "yandexcloud-prod", "yandexcloud-testing", "yandexcloud-stable-lab"]:

            for tag in ["AllRequests", "SlowRequests"]:
                self._analyze_log(folder_prefix, "1d", yesterday, None, tag)

                for hour in hours:
                    self._analyze_log(folder_prefix, "1h", hour, expiration_time, tag)

    def _analyze_log(self, folder_prefix, subfolder, date_str, expiration_time, tag):
        """
        self.ytlib.find_perf_bottlenecks(
            "//home/cloud-nbs/%s/traces/%s/%s" % (folder_prefix, subfolder, date_str),
            "//home/cloud-nbs/%s/analytics/%s" % (folder_prefix, subfolder),
            "hahn",
            expiration_time,
            tag=tag
        )
        """
        describe_result = self.ytlib.describe_slow_requests(
            "//home/cloud-nbs/%s/traces/%s/%s" % (folder_prefix, subfolder, date_str),
            "//home/cloud-nbs/%s/analytics/%s" % (folder_prefix, subfolder),
            "hahn",
            expiration_time,
            tag=tag
        )
        if describe_result is not None and not describe_result.committed():
            report = self.lib.build_report(describe_result.data())
            for request_description, request_report in report:
                resource = NbsPerformanceReport(
                    self.task,
                    "%s performance report %s %s %s" % (request_description, folder_prefix, tag, date_str),
                    "%s_report_%s_%s_%s.html" % (request_description, folder_prefix, tag, date_str),
                    report_tag=folder_prefix,
                    date_str=date_str,
                )
                res_data = ResourceData(resource)
                with open(str(res_data.path), "w") as f:
                    f.write(request_report)
            describe_result.commit()

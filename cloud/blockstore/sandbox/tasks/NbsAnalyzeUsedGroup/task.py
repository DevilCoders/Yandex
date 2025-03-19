from .plot_builder import print_plot

from sandbox.sdk2 import Vault
from sandbox.sdk2 import Resource
from sandbox.sdk2 import ResourceData

import json
import subprocess


class NbsUsedGroupPlot(Resource):
    """ Used group plot """

    releasable = True
    releasers = ['YC_NBS']


class Task():
    def __init__(self, task, logger):
        self.logger = logger.getChild('main')
        self.task = task
        self.core_resource = task.Parameters.core_resource
        self.thread_count = task.Parameters.thread_count
        self.ydb_url = task.Parameters.ydb_url
        self.ydb_database = task.Parameters.ydb_database
        self.ydb_table = task.Parameters.ydb_table
        self.ydb_token = str(Vault.data(
            task.Parameters.ydb_oauth.owner,
            task.Parameters.ydb_oauth.name))

    def run(self):
        resource = Resource[self.core_resource]
        data = ResourceData(resource)
        path = data.path

        for table in self.ydb_table.split():
            core_result = subprocess.run(
                [str(path) + "/nbs_analyze_used_group_core",
                    "--token", self.ydb_token,
                    "--endpoint", self.ydb_url,
                    "--database", self.ydb_database,
                    "--table", table,
                    "--workers", str(self.thread_count)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)

            self.logger.info("Core result returncode %s", str(core_result.returncode))
            self.logger.info("Core result stderr %s", core_result.stderr)

            try:
                data_json = json.loads(core_result.stdout, strict=False)
            except:
                self.logger.info("Core result stdout %s", core_result.stdout)
                continue

            data_array = "<html><head></head><body>"
            for kind in data_json:
                data_array += print_plot(
                    str(table) + " : " + str(kind),
                    data_json[kind]["percentile_array"],
                    data_json[kind]["groups_count"])
            data_array += "</body></html>"

            resource = NbsUsedGroupPlot(
                self.task,
                "Plot for " + table, table + ".html")
            resource.path.write_bytes(data_array)

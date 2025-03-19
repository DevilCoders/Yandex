import logging
import os
import signal
import time

from yatest.common import process

from ydb.tests.library.harness.daemon import Daemon
from ydb.tests.library.harness.kikimr_runner import get_unique_path_for_current_test, ensure_path_exists
import ydb.tests.library.common.yatest_common as yatest_common

logger = logging.getLogger()


DEFAULT_CONFIG_TEMPLATE = """
Hostname: "{hostname}"
TasksConfig: <
    TaskPingPeriod: "1s"
    PollForTaskUpdatesPeriod: "1s"
    PollForTasksPeriodMin: "1s"
    PollForTasksPeriodMax: "2s"
    PollForStallingTasksPeriodMin: "2s"
    PollForStallingTasksPeriodMax: "4s"
    TaskStallingTimeout: "5s"
    TaskWaitingTimeout: "3s"
    ScheduleRegularTasksPeriodMin: "2s"
    ScheduleRegularTasksPeriodMax: "4s"
    RunnersCount: 10
    StalkingRunnersCount: 10
    EndedTaskExpirationTimeout: "200s"
    ClearEndedTasksTaskScheduleInterval: "11s"
    ClearEndedTasksLimit: 10
    MaxRetriableErrorCount: 1000
    HangingTaskTimeout: "100s"
>
PersistenceConfig: <
    Endpoint: "localhost:{kikimr_port}"
    Database: "/Root"
    RootPath: "disk_manager/tasks/acceptance_tests/recipe"
>
LoggingConfig: <
    LoggingStderr: <>
    Level: LEVEL_DEBUG
>
"""


def get_pid_file_name(idx):
    return "disk_manager_tasks_acceptance_tests_recipe_node_{}.pid".format(idx)


class Node(Daemon):

    def __init__(self, config_file, working_dir):
        internal_command = yatest_common.binary_path(
            "cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/node/node")
        internal_command += " --config " + config_file

        command = [yatest_common.binary_path(
            "cloud/disk_manager/test/nemesis/nemesis")]
        command += ["--cmd", internal_command]

        super(Node, self).__init__(
            command=command,
            cwd=working_dir,
            timeout=180)


class NodeLauncher:

    def __init__(self, hostname, kikimr_port, idx):
        self.__idx = idx

        working_dir = get_unique_path_for_current_test(
            output_path=yatest_common.output_path(),
            sub_folder=""
        )
        ensure_path_exists(working_dir)
        config_file = os.path.join(working_dir, "node_config_{}.txt".format(idx))

        self.__config_string = DEFAULT_CONFIG_TEMPLATE.format(
            hostname=hostname,
            kikimr_port=kikimr_port,
        )
        with open(config_file, "w") as f:
            f.write(self.__config_string)

        init_database_command = [
            yatest_common.binary_path(
                "cloud/disk_manager/internal/pkg/tasks/acceptance_tests/recipe/init-db/init-db"
            ),
            "--config",
            config_file,
        ]

        attempts_left = 20
        while True:
            try:
                process.execute(init_database_command)
                break
            except yatest_common.ExecutionError as e:
                logger.error("init_database_command=%s failed with error=%s", init_database_command, e)

                attempts_left -= 1
                if attempts_left == 0:
                    raise e

                time.sleep(1)
                continue

        self.__daemon = Node(config_file, working_dir)

    def start(self):
        self.__daemon.start()
        with open(get_pid_file_name(self.__idx), "w") as f:
            f.write(str(self.__daemon.daemon.process.pid))

    @staticmethod
    def stop(idx):
        pid_file_name = get_pid_file_name(idx)
        if not os.path.exists(pid_file_name):
            return

        with open(pid_file_name) as f:
            pid = int(f.read())
            os.kill(pid, signal.SIGTERM)

    @property
    def config_string(self):
        return self.__config_string

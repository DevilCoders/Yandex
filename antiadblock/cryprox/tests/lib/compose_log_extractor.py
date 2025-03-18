# -*- coding: utf8 -*-

import time

from docker import DockerClient
from docker.errors import APIError


DOCKER_CLIENT = DockerClient(version='auto')


def pytest_configure(config):
    config.pluginmanager.register(ComposeLogExtractor())


class ComposeLogExtractor(object):
    """
    Automatically record all logs for the test duration from running containers if test has failed
    """

    def pytest_runtest_call(self, item):
        self.test_start = int(time.time())
        self.containers = []
        if 'containers_context' in item.fixturenames:
            cryprox_container, nginx_container = item.funcargs["containers_context"]
            self.containers = [cryprox_container.short_id, nginx_container.short_id]

    def pytest_runtest_logreport(self, report):
        if report.failed and report.when != "setup":
            for container in DOCKER_CLIENT.containers.list(all=True):
                if container.short_id in self.containers or not self.containers:
                    try:
                        logs = container.logs(since=self.test_start)
                    except APIError:
                        pass  # some of the docker drivers does not support log reading
                    else:
                        report.sections.append((container.name, logs))

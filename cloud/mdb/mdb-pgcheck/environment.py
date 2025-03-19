#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import docker

from steps import moby


def before_scenario(context, scenario):
    """
    Setup environment

    Actually it's enough to run this in before_all but arcadia behave doesn't support it
    """
    # Connect to docker daemon
    context.docker = docker.from_env()

    context.containers = {}
    for container in context.docker.containers.list():
        name = ''.join(container.name.split('_')[1:-1])
        context.containers[name] = container

    try:
        context.plproxy_port = moby.get_container_tcp_port(
            moby.get_container_by_name('plproxy'), 6432)
    except AttributeError:
        raise Exception("PL/Proxy container probably did not start")

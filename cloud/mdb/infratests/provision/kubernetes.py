# -*- coding: utf-8 -*-
from typing import List, NamedTuple
from kubernetes import client as k8s_client, config as k8s_config

from cloud.mdb.infratests.config import InfratestConfig


class LoadBalancer(NamedTuple):
    name: str
    ip: str


def list_external_load_balancers(config: InfratestConfig) -> List[LoadBalancer]:
    balancers = []
    k8s_config.load_kube_config()
    v1 = k8s_client.CoreV1Api()
    ret = v1.list_namespaced_service(namespace=config.stand_name)
    for service in ret.items:
        ingresses = service.status.load_balancer.ingress
        if ingresses:
            balancers.append(LoadBalancer(name=service.metadata.name, ip=ingresses[0].ip))
    return balancers

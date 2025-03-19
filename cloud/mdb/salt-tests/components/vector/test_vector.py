#!/usr/bin/python3
# coding: utf-8

import os.path
import logging
import subprocess

from jinja2 import Template
from typing import List

import yatest.common

COMMON_CONFIG_FILES = [
    'cloud/mdb/salt/salt/components/vector_agent_dataplane/conf/vector.toml',
    'cloud/mdb/salt/salt/components/vector_agent_dataplane/conf/test_vector.toml',
    'cloud/mdb/salt/salt/components/vector_agent_dataplane/conf/vector_billing.toml',
    'cloud/mdb/salt/salt/components/vector_agent_dataplane/conf/vector_billing_test.toml'
]

RENDER_ARGS = {
    'salt': {
        'pillar.get': lambda path, default=None: {
            'data:dbaas:shard_id': 'shardid1',
            'data:dbaas:shard_name': 's1',
            'data:dbaas:vtype': 'vtype',
            'data:dbaas:vtype_id': 'vtype_id',
        }.get(path, default),
        'dbaas.pillar': lambda path: {
            'data:vector:address': "vector-aggregator:12345",
            'data:prometheus:address': "vector-aggregator:54321",
            'data:dbaas:fqdn': "fqnd",
            'data:dbaas:region': 'eu-central-1',
            'data:dbaas:cluster_id': 'cid1',
            'data:dbaas:cluster_type': 'ch',
            'data:dbaas:cluster_name': 'awesome',
            'data:dbaas:cloud_provider': 'aws',
            'data:dbaas:subcluster_id': 'subcid1',
            'data:dbaas:subcluster_name': 'sub1',
            'data:dbaas:cloud:cloud_ext_id': 'cloud1',
            'data:dbaas:folder:folder_ext_id': 'folder1',
        }[path]
    }
}


def _render_config_file(path: str, tmp_root: str) -> str:
    filename = os.path.join(tmp_root, os.path.split(path)[1])
    with open(yatest.common.source_path(path)) as f:
        rendered = Template(f.read()).render(RENDER_ARGS)
    with open(filename, 'w') as f:
        f.write(rendered)
    return filename


def _vector_validate(test_files: List[str]):
    vector = yatest.common.build_path('cloud/mdb/salt-tests/components/vector/vector/vector')
    process = subprocess.Popen([vector, 'validate', '--no-environment', '-d'] + test_files,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    assert process.returncode == 0, f'Validation failed:\nstdout: {stdout.decode()}\nstderr: {stderr.decode()}'


def _vector_test(test_files: List[str]):
    vector = yatest.common.build_path('cloud/mdb/salt-tests/components/vector/vector/vector')
    process = subprocess.Popen([vector, 'test'] + test_files, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    assert process.returncode == 0, f'Some tests had failed:\nstdout: {stdout.decode()}\nstderr: {stderr.decode()}'


def _test_config(specific_files: List[str]):
    tmp_root = os.path.join(yatest.common.ram_drive_path() or yatest.common.work_path(), 'tmp')
    logging.info(f'Using tmp dir: {tmp_root}')
    os.makedirs(tmp_root, exist_ok=True)

    test_files = []
    for file in COMMON_CONFIG_FILES + specific_files:
        test_files.append(_render_config_file(file, tmp_root))

    _vector_validate(test_files)
    _vector_test(test_files)


def test_kafka_config():
    _test_config([
        'cloud/mdb/salt/salt/components/kafka/conf/vector_kafka.toml',
    ])


def test_clickhouse_config():
    _test_config([
        'cloud/mdb/salt/salt/components/clickhouse/vector/conf/vector_clickhouse.toml',
        'cloud/mdb/salt/salt/components/clickhouse/vector/conf/vector_clickhouse_test.toml',
    ])

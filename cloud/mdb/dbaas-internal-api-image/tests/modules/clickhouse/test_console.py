"""
Tests for Console module.
"""
from flask import Flask

from dbaas_internal_api.modules.clickhouse.console import fill_updatable_to
from dbaas_internal_api.utils.register import CLUSTER_TRAITS, register_cluster_traits
from dbaas_internal_api.utils.version import Version


def test_fill_updatable_to():
    app = Flask(__name__)
    with app.app_context():
        app.config['VERSIONS'] = {'TEST_CLUSTER_TYPE': []}

        @register_cluster_traits('TEST_CLUSTER_TYPE')
        class TestClusterTraits:
            versions = [
                {'version': '3'},
                {'version': '4', 'downgradable': False},
                {'version': '5'},
            ]

        versions = [
            {'id': Version(major=3)},
            {'id': Version(major=4)},
            {'id': Version(major=5)},
        ]
        correct_updates = [[Version(major=5), Version(major=4)], [Version(major=5)], [Version(major=4)]]
        fill_updatable_to('TEST_CLUSTER_TYPE', versions)
        assert all(map(lambda x: x[0] == x[1], zip(map(lambda ver: ver['updatable_to'], versions), correct_updates)))
        CLUSTER_TRAITS.pop('TEST_CLUSTER_TYPE')

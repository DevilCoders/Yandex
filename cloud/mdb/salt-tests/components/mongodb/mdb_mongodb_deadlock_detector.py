from mdb_mongodb_deadlock_detector_mocks import initMockMongoDeadlockDetectorConfig
import cloud.mdb.salt.salt.components.mongodb.conf.mdb_mongodb_deadlock_detector as mdb_mongodb_deadlock_detector

import sys
from cloud.mdb.internal.python.pytest.utils import parametrize


def assert_structures_are_equal(data1, data2):
    if data1 != data2:
        print("====================", file=sys.stderr)
        print(data1, file=sys.stderr)
        print(data2, file=sys.stderr)
    assert data1 == data2
    return True


TEST_DATA = [  # config, argv, initial_data, expected_data
    {
        'id': 'FailCheck -> No action',
        'args': {
            'config': {
                'config_file': None,
                'helpers': {'mongodb': {'mongouri': ''}},
                'checks': {
                    'active': ['fail'],
                    'fail': {'check': 'fail'},
                    'pass': {'check': 'pass'},
                },
                'actions': {
                    'active': ['set_action'],
                    'set_action': {'action': 'set_action'},
                },
            },
            'argv': [],
            'initial_data': {},
            'expected': {},
        },
    },
    {
        'id': 'PassCheck -> With action',
        'args': {
            'config': {
                'config_file': None,
                'helpers': {'mongodb': {'mongouri': ''}},
                'checks': {
                    'active': ['pass'],
                    'fail': {'check': 'fail'},
                    'pass': {'check': 'pass'},
                },
                'actions': {
                    'active': ['set_action'],
                    'set_action': {'action': 'set_action'},
                },
            },
            'argv': [],
            'initial_data': {},
            'expected': {
                'action': True,
            },
        },
    },
    {
        'id': 'PassCheck, --dry-run -> No action',
        'args': {
            'config': {
                'config_file': None,
                'helpers': {'mongodb': {'mongouri': ''}},
                'checks': {
                    'active': ['pass'],
                    'fail': {'check': 'fail'},
                    'pass': {'check': 'pass'},
                },
                'actions': {
                    'active': ['set_action'],
                    'set_action': {'action': 'set_action'},
                },
            },
            'argv': ['--dry-run'],
            'initial_data': {},
            'expected': {},
        },
    },
]


@parametrize(*TEST_DATA)
def test_mdb_mongodb_deadlock_detector(config, argv, initial_data, expected):
    app = initMockMongoDeadlockDetectorConfig(initial_data, config)
    mdb_mongodb_deadlock_detector.processArguments(app['config'], argv)
    mdb_mongodb_deadlock_detector.processChecksAndActions(app['config'], app['checks'], app['actions'], app['helpers'])

    assert_structures_are_equal(app['test_data_container'], expected)

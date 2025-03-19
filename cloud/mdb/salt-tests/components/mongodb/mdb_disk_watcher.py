from mdb_disk_watcher_mocks import MockMDBMongoDiskWatcher

import sys
from cloud.mdb.internal.python.pytest.utils import parametrize


def assert_structures_are_equal(data1, data2):
    if data1 != data2:
        print("====================", file=sys.stderr)
        print(data1, file=sys.stderr)
        print(data2, file=sys.stderr)
    assert data1 == data2
    return True


MBYTE = 1024**2
TEST_DATA = [  # initial data, argv, expected data
    {
        'id': '600 of 1000 Mb Free, no need to lock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': '600 of 1000MB Free, MongoDB locked, need to unlock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Free 50% but need 51%, need to lock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 10000 * MBYTE,
                    'free': 5000 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': ['--free-percent-limit', '51'],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 10000 * MBYTE,
                    'free': 5000 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Free 500MB but Need 510, need to lock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': ['--free-mb-limit', '510'],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'MongoDB Locked by hands, no need to unlock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Unlocked mongodb marked as locked, but need to be unlocked, need fix mark',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Locked MongoDB restarted, need to relock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 2,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 2,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 2,
                },
            },
        },
    },
    {
        'id': 'Lock and unlock files exists, recheck needed  (MongoDB unlocked, enough space)',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Lock and unlock files exists, recheck needed  (MongoDB locked, enough space)',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 600 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Lock and unlock files exists, recheck needed  (MongoDB unlocked, not enough space)',
        'args': {
            'initial_data': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'Lock and unlock files exists, recheck needed  (MongoDB locked, not enough space)',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': [],
            'expected': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'MongoDB locked, not enough space, trying to --unlock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': ['--unlock'],
            'expected': {
                'mongodb': {'locked': False, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'MongoDB locked, not enough space, trying to --unfreeze',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': ['--unfreeze'],
            'expected': {
                'mongodb': {'locked': True, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': None,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
    {
        'id': 'MongoDB locked, not enough space, trying to --unfreeze and --unlock',
        'args': {
            'initial_data': {
                'mongodb': {'locked': True, 'frozen': True},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': 1,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
            'argv': ['--unlock', '--unfreeze'],
            'expected': {
                'mongodb': {'locked': False, 'frozen': False},
                'fs': {
                    'space': 1000 * MBYTE,
                    'free': 500 * MBYTE,
                },
                'files': {
                    '/tmp/mdb-mongo-fsync.locked': None,
                    '/tmp/mdb-mongo-fsync.unlocked': 1,
                    '/var/run/mongodb.pid': 1,
                },
            },
        },
    },
]


@parametrize(*TEST_DATA)
def test_mdb_disk_watcher(initial_data, argv, expected):
    app = MockMDBMongoDiskWatcher(initial_data)
    app.main(argv)

    assert_structures_are_equal(app.get_test_data_container(), expected)

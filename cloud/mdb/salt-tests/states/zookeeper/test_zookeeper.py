# coding: utf-8

from kazoo.security import make_digest_acl

from cloud.mdb.salt.salt._states import zookeeper
from cloud.mdb.internal.python.pytest.utils import parametrize

DEFAULT_ACL = [
    {
        'username': 'default',
        'password': 'default_pass',
        'write': True,
    }
]


@parametrize(
    {
        'id': 'Create zookeeper node',
        'args': {
            'name': 'test_node',
            'value': 'test_node_value',
            'acls': [
                {
                    'username': 'test_user',
                    'password': 'pass',
                    'write': True,
                }
            ],
            'zk_data': {},
            'test': False,
            'result': {
                'name': 'test_node',
                'result': True,
                'comment': 'Znode test_node successfully created',
                'changes': {
                    'old': {},
                    'new': {'value': 'test_node_value', 'acls': [make_digest_acl('test_user', 'pass', write=True)]},
                },
            },
        },
    },
    {
        'id': 'Create zookeeper node with test',
        'args': {
            'name': 'test_node',
            'value': 'test_node_value',
            'acls': [
                {
                    'username': 'test_user',
                    'password': 'pass',
                    'write': True,
                }
            ],
            'zk_data': {},
            'test': True,
            'result': {
                'name': 'test_node',
                'result': None,
                'comment': 'Znode test_node is will be created',
                'changes': {
                    'old': {},
                    'new': {'value': 'test_node_value', 'acls': [make_digest_acl('test_user', 'pass', write=True)]},
                },
            },
        },
    },
    {
        'id': 'Update existing zookeeper node',
        'args': {
            'name': 'test_node',
            'value': 'test_node_value',
            'acls': [
                {
                    'username': 'test_user',
                    'password': 'pass',
                    'write': True,
                }
            ],
            'zk_data': {
                'test_node': {
                    'value': 'old_node_value',
                    'acls': [make_digest_acl('old_user', 'old_pass', all=True)],
                }
            },
            'test': False,
            'result': {
                'name': 'test_node',
                'result': True,
                'comment': 'Znode test_node successfully updated',
                'changes': {
                    'old': {'value': 'old_node_value', 'acls': [make_digest_acl('old_user', 'old_pass', all=True)]},
                    'new': {'value': 'test_node_value', 'acls': [make_digest_acl('test_user', 'pass', write=True)]},
                },
            },
        },
    },
    {
        'id': 'Update existing zookeeper node',
        'args': {
            'name': 'test_node',
            'value': 'test_node_value',
            'acls': [
                {
                    'username': 'test_user',
                    'password': 'pass',
                    'write': True,
                }
            ],
            'zk_data': {
                'test_node': {
                    'value': 'old_node_value',
                    'acls': [make_digest_acl('old_user', 'old_pass', all=True)],
                }
            },
            'test': True,
            'result': {
                'name': 'test_node',
                'result': None,
                'comment': 'Znode test_node is will be updated',
                'changes': {
                    'old': {'value': 'old_node_value', 'acls': [make_digest_acl('old_user', 'old_pass', all=True)]},
                    'new': {'value': 'test_node_value', 'acls': [make_digest_acl('test_user', 'pass', write=True)]},
                },
            },
        },
    },
)
def test_zookeeper_present(name, value, acls, zk_data, test, result):
    zookeeper.__salt__['zookeeper.make_digest_acl'] = make_digest_acl
    zookeeper.__salt__['zookeeper.exists'] = lambda key, *args, **kwargs: key in zk_data
    zookeeper.__salt__['zookeeper.get'] = lambda key, *args, **kwargs: zk_data[key]['value']
    zookeeper.__salt__['zookeeper.get_acls'] = lambda key, **kwargs: zk_data[key]['acls']
    zookeeper.__salt__['zookeeper.set'] = lambda key, val, *args, **kwargs: zk_data.get(
        key, {'acls': _make_digest_acls(DEFAULT_ACL)}
    ).update({'value': val}) or zk_data.get(key, {})
    zookeeper.__salt__['zookeeper.set_acls'] = lambda key, new_acls, *args, **kwargs: zk_data.get(key, {}).update(
        {'acls': _make_digest_acls(new_acls)}
    ) or zk_data.get(key, {})
    zookeeper.__salt__['zookeeper.create'] = lambda key, val, new_acls, *args, **kwargs: zk_data.update(
        {key: {'value': val, 'acls': _make_digest_acls(DEFAULT_ACL if new_acls is None else new_acls)}}
    )

    zookeeper.__opts__['test'] = test

    assert zookeeper.present(name, value, acls) == result


def _make_digest_acls(acls):
    return [make_digest_acl(**acl) for acl in acls]

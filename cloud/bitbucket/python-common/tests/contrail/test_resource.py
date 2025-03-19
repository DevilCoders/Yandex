import json

import pytest
from unittest import mock

from yc_common.clients.contrail.exceptions import HttpError

from yc_common.clients.contrail.client import ContrailAPISession
from yc_common.clients.contrail.context import Context
from yc_common.clients.contrail.resource import RootCollection, Collection, Resource, ResourceEncoder
from yc_common.clients.contrail.exceptions import ResourceNotFoundError, ResourceMissingError, CollectionNotFoundError
from yc_common.clients.contrail.schema import DummySchema
from yc_common.clients.contrail.utils import Path, FQName


@pytest.fixture
def base_url():
    return "http://localhost:8082"


@pytest.fixture(scope='function')
def contrail_context(request, base_url):
    context = Context()
    context.session = mock.MagicMock(base_url=base_url)
    context.session.get_json.return_value = {
        "href": base_url,
        "links": [
            {"link": {"href": base_url + "/foos", "name": "foo", "rel": "collection"}},
            {"link": {"href": base_url + "/bars", "name": "bar", "rel": "collection"}},
            {"link": {"href": base_url + "/foobars", "name": "foobar", "rel": "collection"}},
        ],
    }
    context.schema = DummySchema(context)

    return context


def test_resource_base(contrail_context, base_url):
    c = RootCollection(contrail_context)
    assert c.uuid == ''
    assert c.fq_name == FQName()
    assert c.href == base_url + '/'

    c = Collection(contrail_context, 'foo')
    assert c.uuid == ''
    assert c.fq_name == FQName()
    assert c.href == base_url + '/foos'

    r = Resource(contrail_context, 'foo', uuid='x')
    assert r.uuid == 'x'
    assert r.fq_name == FQName()
    assert r.href == base_url + '/foo/x'

    r.update({'uuid': 'xxx'})
    assert r.uuid == 'xxx'

    r = Resource(contrail_context, 'foo', fq_name='foo:bar')
    assert r.uuid == ''
    assert r.fq_name == FQName('foo:bar')
    assert r.href == base_url + '/foos'

    r = Resource(contrail_context, 'foo', to='foo:bar')
    assert r.uuid == ''
    assert r.fq_name == FQName('foo:bar')
    assert r.href == base_url + '/foos'

    with pytest.raises(AssertionError):
        r = Resource(contrail_context, 'bar')


def test_root_collection(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)
    contrail_context.session.get_json.return_value = {
        "href": base_url,
        "links": [
            {"link": {"href": base_url + "/instance-ips", "name": "instance-ip", "rel": "collection"}},
            {"link": {"href": base_url + "/instance-ip", "name": "instance-ip", "rel": "resource-base"}},
        ],
    }
    root_collection = RootCollection(contrail_context, fetch=True)

    assert not root_collection.href.endswith('s')

    expected_root_resources = RootCollection(contrail_context, data=[Collection(contrail_context, 'instance-ip')])
    assert root_collection == expected_root_resources


def test_resource_collection(contrail_context, base_url):
    contrail_context.session.get_json.return_value = {
        "foos": [
            {
                "href": base_url + "/foo/ec1afeaa-8930-43b0-a60a-939f23a50724",
                "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
            },
            {
                "href": base_url + "/foo/c2588045-d6fb-4f37-9f46-9451f653fb6a",
                "uuid": "c2588045-d6fb-4f37-9f46-9451f653fb6a",
            },
        ]
    }

    collection = Collection(contrail_context, 'foo', fetch=True)

    assert collection.href.endswith('s')
    assert collection.fq_name == FQName()

    expected_collection = Collection(contrail_context, 'foo')
    expected_collection.data = [
        Resource(
            contrail_context,
            'foo',
            href=base_url + "/foo/ec1afeaa-8930-43b0-a60a-939f23a50724",
            uuid="ec1afeaa-8930-43b0-a60a-939f23a50724",
        ),
        Resource(
            contrail_context,
            'foo',
            href=base_url + "/foo/c2588045-d6fb-4f37-9f46-9451f653fb6a",
            uuid="c2588045-d6fb-4f37-9f46-9451f653fb6a",
        ),
    ]
    assert collection == expected_collection


def test_resource_fqname(contrail_context):
    r = Resource(contrail_context, 'foo', fq_name=['domain', 'foo', 'uuid'])
    assert str(r.fq_name) == 'domain:foo:uuid'
    r = Resource(contrail_context, 'foo', to=['domain', 'foo', 'uuid'])
    assert str(r.fq_name) == 'domain:foo:uuid'


def test_resource(contrail_context, base_url):
    # bind original method to contrail_context.session
    contrail_context.session.make_url = ContrailAPISession.make_url.__get__(contrail_context.session)
    contrail_context.session.get_json.return_value = {
        "foo": {
            "href": base_url + "/foo/ec1afeaa-8930-43b0-a60a-939f23a50724",
            "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
            "attr": None,
            "fq_name": ["foo", "ec1afeaa-8930-43b0-a60a-939f23a50724"],
            "bar_refs": [
                {
                    "href": base_url + "/bar/15315402-8a21-4116-aeaa-b6a77dceb191",
                    "uuid": "15315402-8a21-4116-aeaa-b6a77dceb191",
                    "to": ["bar", "15315402-8a21-4116-aeaa-b6a77dceb191"],
                }
            ],
        }
    }
    resource = Resource(contrail_context, "foo", uuid="ec1afeaa-8930-43b0-a60a-939f23a50724", fetch=True)

    expected_resource = Resource(
        contrail_context,
        "foo",
        uuid="ec1afeaa-8930-43b0-a60a-939f23a50724",
        href=base_url + "/foo/ec1afeaa-8930-43b0-a60a-939f23a50724",
        attr=None,
        fq_name=["foo", "ec1afeaa-8930-43b0-a60a-939f23a50724"],
        bar_refs=[
            {
                'uuid': "15315402-8a21-4116-aeaa-b6a77dceb191",
                'href': base_url + "/bar/15315402-8a21-4116-aeaa-b6a77dceb191",
                'to': ["bar", "15315402-8a21-4116-aeaa-b6a77dceb191"],
            }
        ],
    )
    assert resource == expected_resource


def test_resource_fqname_validation(contrail_context, base_url):
    # bind original method to contrail_context.session
    contrail_context.session.fqname_to_id = ContrailAPISession.fqname_to_id.__get__(contrail_context.session)
    contrail_context.session.post_json = ContrailAPISession.post_json.__get__(contrail_context.session)
    contrail_context.session.make_url = ContrailAPISession.make_url.__get__(contrail_context.session)

    # called by fqname_to_id
    def post(url, data=None, headers=None):
        data = json.loads(data)
        result = mock.Mock()
        if data['type'] == "foo":
            result.json.return_value = {"uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724"}
            return result
        if data['type'] == "bar":
            raise HttpError(http_status=404)

    contrail_context.session.post.side_effect = post
    r = Resource(contrail_context, 'foo', fq_name='domain:foo:uuid', check=True)
    assert r.uuid == 'ec1afeaa-8930-43b0-a60a-939f23a50724'
    assert r.path == Path('/foo/ec1afeaa-8930-43b0-a60a-939f23a50724')

    with pytest.raises(ResourceNotFoundError) as e:
        r = Resource(contrail_context, 'bar', fq_name='domain:bar:nofound', check=True)
        assert str(e) == "Resource domain:bar:nofound doesn't exists"


def test_resource_uuid_validation(contrail_context, base_url):
    # bind original method to contrail_context.session
    contrail_context.session.id_to_fqname = ContrailAPISession.id_to_fqname.__get__(contrail_context.session)
    contrail_context.session.make_url = ContrailAPISession.make_url.__get__(contrail_context.session)

    # called by id_to_fqname
    def post(url, json):
        if json['uuid'] == 'a5a1b67b-4246-4e2d-aa24-479d8d47435d':
            return {'type': 'foo', 'fq_name': ['domain', 'foo', 'uuid']}
        else:
            raise HttpError(http_status=404)

    contrail_context.session.post_json.side_effect = post
    r = Resource(contrail_context, 'foo', uuid='a5a1b67b-4246-4e2d-aa24-479d8d47435d', check=True)
    assert str(r.fq_name) == 'domain:foo:uuid'
    assert r.path == Path('/foo/a5a1b67b-4246-4e2d-aa24-479d8d47435d')
    with pytest.raises(ResourceNotFoundError) as e:
        r = Resource(contrail_context, 'bar', uuid='d6e9fae3-628c-448c-bfc5-849d82a9a016', check=True)
        assert str(e) == "Resource d6e9fae3-628c-448c-bfc5-849d82a9a016 doesn't exists"


def test_resource_fetch(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)

    contrail_context.session.fqname_to_id.side_effect = HttpError(http_status=404)
    r = Resource(contrail_context, 'foo', fq_name='domain:bar:foo')
    with pytest.raises(ResourceNotFoundError):
        r.fetch()

    contrail_context.session.fqname_to_id.side_effect = ["07eeb3c0-42f5-427c-9409-6ae45b376aa2"]
    contrail_context.session.get_json.return_value = {
        'foo': {'uuid': '07eeb3c0-42f5-427c-9409-6ae45b376aa2', 'fq_name': ['domain', 'bar', 'foo']}
    }
    r = Resource(contrail_context, 'foo', fq_name='domain:bar:foo')
    r.fetch()
    assert r.uuid == '07eeb3c0-42f5-427c-9409-6ae45b376aa2'
    contrail_context.session.get_json.assert_called_with(r.href)

    r.fetch(exclude_children=True)
    contrail_context.session.get_json.assert_called_with(r.href, exclude_children=True)

    r.fetch(exclude_back_refs=True)
    contrail_context.session.get_json.assert_called_with(r.href, exclude_back_refs=True)


def test_resource_save(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)
    contrail_context.session.fqname_to_id.return_value = '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd'
    contrail_context.session.get_json.side_effect = [
        {'foo': {'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd', 'attr': 'bar', 'fq_name': ['domain', 'bar', 'foo']}},
        {'foo': {'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd', 'attr': 'foo', 'fq_name': ['domain', 'bar', 'foo']}},
    ]

    r1 = Resource(contrail_context, 'foo', fq_name='domain:bar:foo')
    r1['attr'] = 'bar'
    r1.save()
    contrail_context.session.post_json.assert_called_with(
        base_url + '/foos', {'foo': {'attr': 'bar', 'fq_name': ['domain', 'bar', 'foo']}}, cls=ResourceEncoder
    )
    assert r1['attr'] == 'bar'

    r1['attr'] = 'foo'
    r1.save()
    contrail_context.session.put_json.assert_called_with(
        base_url + '/foo/8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd',
        {'foo': {'attr': 'foo', 'fq_name': ['domain', 'bar', 'foo'], 'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd'}},
        cls=ResourceEncoder,
    )
    assert r1['attr'] == 'foo'


def test_resource_create(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)
    contrail_context.session.fqname_to_id.return_value = '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd'
    contrail_context.session.get_json.side_effect = [
        {'foo': {'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd', 'attr': 'bar', 'fq_name': ['domain', 'bar', 'foo']}},
        {'foo': {'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd', 'attr': 'foo', 'fq_name': ['domain', 'bar', 'foo']}},
    ]

    r1 = Resource(contrail_context, 'foo', fq_name='domain:bar:foo', uuid='8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd')
    r1['attr'] = 'bar'
    r1.create()
    contrail_context.session.post_json.assert_called_with(
        base_url + '/foos',
        {'foo': {'attr': 'bar', 'fq_name': ['domain', 'bar', 'foo'], 'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd'}},
        cls=ResourceEncoder,
    )
    assert r1['attr'] == 'bar'

    r1['attr'] = 'foo'
    r1.create()
    contrail_context.session.post_json.assert_called_with(
        base_url + '/foos',
        {'foo': {'attr': 'foo', 'fq_name': ['domain', 'bar', 'foo'], 'uuid': '8240f9c7-0f28-4ca7-b92e-2f6441a0a6dd'}},
        cls=ResourceEncoder,
    )
    assert r1['attr'] == 'foo'


def test_resource_parent(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)

    p = Resource(contrail_context, 'bar', uuid='57ef609c-6c9b-4b91-a542-26c61420c37b')
    r = Resource(contrail_context, 'foo', fq_name='domain:foo', parent=p)
    assert p == r.parent

    contrail_context.session.id_to_fqname.side_effect = HttpError(http_status=404)
    with pytest.raises(ResourceNotFoundError):
        p = Resource(contrail_context, 'foobar', uuid='1fe29f52-28dc-44a5-90d0-43de1b02cbd8')
        Resource(contrail_context, 'foo', fq_name='domain:foo', parent=p)

    with pytest.raises(ResourceMissingError):
        r = Resource(contrail_context, 'foo', fq_name='domain:foo')
        r.parent


def test_resource_check(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)

    contrail_context.session.id_to_fqname.side_effect = HttpError(http_status=404)
    r = Resource(contrail_context, 'bar', uuid='57ef609c-6c9b-4b91-a542-26c61420c37b')
    assert not r.exists

    contrail_context.session.id_to_fqname.side_effect = [{'type': 'bar', 'fq_name': FQName('domain:bar')}]
    r = Resource(contrail_context, 'bar', uuid='57ef609c-6c9b-4b91-a542-26c61420c37b')
    assert r.exists

    contrail_context.session.fqname_to_id.side_effect = HttpError(http_status=404)
    r = Resource(contrail_context, 'bar', fq_name='domain:foo')
    assert not r.exists

    contrail_context.session.fqname_to_id.side_effect = ['588e1a17-ae50-4b67-8078-95f061d833ca']
    assert r.exists


def test_resource_ref_update(contrail_context, base_url):
    contrail_context.session.configure_mock(base_url=base_url)
    contrail_context.session.make_url = ContrailAPISession.make_url.__get__(contrail_context.session)
    contrail_context.session.add_ref = ContrailAPISession.add_ref.__get__(contrail_context.session)
    contrail_context.session.remove_ref = ContrailAPISession.remove_ref.__get__(contrail_context.session)
    contrail_context.session._ref_update = ContrailAPISession._ref_update.__get__(contrail_context.session)

    r1 = Resource(contrail_context, 'foo', uuid='2caf30aa-d197-40be-82dc-3bac4ca91adb', fq_name='domain:foo')
    r2 = Resource(contrail_context, 'bar', uuid='5d085b74-2dcc-4180-8284-10a56f9ed318', fq_name='domain:bar')

    contrail_context.session.get_json.return_value = {
        'foo': {
            'uuid': '2caf30aa-d197-40be-82dc-3bac4ca91adb',
            'fq_name': ['domain', 'foo'],
            'bar_refs': [
                {'attr': {'prop': 'value'}, 'to': ['domain', 'bar'], 'uuid': '5d085b74-2dcc-4180-8284-10a56f9ed318'}
            ],
        }
    }
    r1.add_ref(r2, attr={'prop': 'value'})
    data = {
        'type': r1.type,
        'uuid': r1.uuid,
        'ref-type': r2.type,
        'ref-fq-name': list(r2.fq_name),
        'ref-uuid': r2.uuid,
        'operation': 'ADD',
        'attr': {'prop': 'value'},
    }
    contrail_context.session.post_json.assert_called_with(base_url + '/ref-update', data)
    assert r1['bar_refs'][0].uuid, r2.uuid

    contrail_context.session.get_json.return_value = {
        'foo': {'uuid': '2caf30aa-d197-40be-82dc-3bac4ca91adb', 'fq_name': ['domain', 'foo']}
    }
    r1.remove_ref(r2)
    data = {
        'type': r1.type,
        'uuid': r1.uuid,
        'ref-type': r2.type,
        'ref-fq-name': list(r2.fq_name),
        'ref-uuid': r2.uuid,
        'operation': 'DELETE',
        'attr': None,
    }
    contrail_context.session.post_json.assert_called_with(base_url + '/ref-update', data)
    assert 'bar_refs' not in r1


def test_collection_fields(contrail_context, base_url):
    contrail_context.session = mock.MagicMock()
    contrail_context.session.configure_mock(base_url=base_url)
    c = Collection(contrail_context, 'foo', fields=['foo', 'bar'], fetch=True)
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', fields='foo,bar')
    c.fetch(fields=['baz'])
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', fields='foo,bar,baz')
    c.fetch()
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', fields='foo,bar')


def test_collection_filters(contrail_context, base_url):
    contrail_context.session = mock.MagicMock()
    contrail_context.session.configure_mock(base_url=base_url)
    c = Collection(contrail_context, 'foo', filters=[('foo', 'bar')], fetch=True)
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', filters='foo=="bar"')
    c.fetch(filters=[('bar', False)])
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', filters='foo=="bar",bar==false')
    c.fetch()
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', filters='foo=="bar"')
    c.filter('bar', 42)
    c.fetch()
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', filters='foo=="bar",bar==42')


def test_collection_detail(contrail_context, base_url):
    contrail_context.session = mock.MagicMock()
    contrail_context.session.configure_mock(base_url=base_url)
    c = Collection(contrail_context, 'foo', fields=['foo', 'bar'], detail=True, fetch=True)
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', detail=True)
    c.fetch(fields=['baz'])
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', detail=True)
    c.fetch()
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', detail=True)
    c = Collection(contrail_context, 'foo', detail=True, fetch=True)
    contrail_context.session.get_json.assert_called_with(c.href, detail=True)
    c = Collection(contrail_context, 'foo', detail=False, fetch=True)
    contrail_context.session.get_json.assert_called_with(c.href)
    c = Collection(contrail_context, 'foo', detail='other', fetch=True)
    contrail_context.session.get_json.assert_called_with(c.href)

    contrail_context.session.get_json.return_value = {
        'foos': [
            {
                'foo': {
                    'uuid': 'dd2f4111-abda-405f-bce9-c6c24181dd14',
                    'bar_refs': [{'uuid': '3042be83-a5f7-4b94-a2a4-9e2ae7fe25be'}],
                }
            },
            {
                'foo': {
                    'uuid': '9fe7094d-f54e-4284-a813-9ca4df866019',
                }
            },
        ]
    }
    c = Collection(contrail_context, 'foo', detail=True, fetch=True)
    assert c.data[0]['bar_refs'][0] == Resource(contrail_context, 'bar', uuid='3042be83-a5f7-4b94-a2a4-9e2ae7fe25be')
    assert c.data[1] == Resource(contrail_context, 'foo', uuid='9fe7094d-f54e-4284-a813-9ca4df866019')


def test_collection_parent_uuid(contrail_context, base_url):
    contrail_context.session = mock.MagicMock()
    contrail_context.session.configure_mock(base_url=base_url)
    c = Collection(contrail_context, 'foo', parent_uuid='aa')
    assert c.parent_uuid == []
    c = Collection(contrail_context, 'foo', parent_uuid='0d7d4197-891b-4767-b599-54667370cab1')
    assert c.parent_uuid == ['0d7d4197-891b-4767-b599-54667370cab1']
    c = Collection(
        contrail_context,
        'foo',
        parent_uuid=['0d7d4197-891b-4767-b599-54667370cab1', '3a0e179e-fbe6-4390-8e5d-00a630de0b68'],
    )
    assert c.parent_uuid == ['0d7d4197-891b-4767-b599-54667370cab1', '3a0e179e-fbe6-4390-8e5d-00a630de0b68']
    c.fetch()
    expected_parent_id = '0d7d4197-891b-4767-b599-54667370cab1,3a0e179e-fbe6-4390-8e5d-00a630de0b68'
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', parent_id=expected_parent_id)
    c.fetch(parent_uuid='a9420bd1-59dc-4576-a548-b28cedbf3e5c')
    expected_parent_id = (
        '0d7d4197-891b-4767-b599-54667370cab1,3a0e179e-fbe6-4390-8e5d-00a630de0b68,a9420bd1-59dc-4576-a548-b28cedbf3e5c'
    )
    contrail_context.session.get_json.assert_called_with(base_url + '/foos', parent_id=expected_parent_id)


def test_collection_count(contrail_context, base_url):
    contrail_context.session = mock.MagicMock()
    contrail_context.session.configure_mock(base_url=base_url)
    contrail_context.session.get_json.return_value = {"foos": {"count": 2}}

    c = Collection(contrail_context, 'foo')
    assert len(c) == 2
    expected_calls = [
        mock.call(base_url + '/foos', count=True),
    ]
    assert contrail_context.session.get_json.mock_calls == expected_calls

    contrail_context.session.get_json.return_value = {
        "foos": [
            {
                "href": base_url + "/foo/ec1afeaa-8930-43b0-a60a-939f23a50724",
                "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
            },
            {
                "href": base_url + "/foo/c2588045-d6fb-4f37-9f46-9451f653fb6a",
                "uuid": "c2588045-d6fb-4f37-9f46-9451f653fb6a",
            },
        ]
    }
    c.fetch()
    assert len(c) == 2
    expected_calls.append(mock.call(base_url + '/foos'))
    assert contrail_context.session.get_json.mock_calls == expected_calls


def test_collection_contrail_name(contrail_context):
    c = Collection(contrail_context, '')
    assert c._contrail_name == ''
    c = Collection(contrail_context, 'foo')
    assert c._contrail_name == 'foos'


def test_collection_not_found(contrail_context):
    contrail_context.session.get_json.side_effect = HttpError(http_status=404)
    with pytest.raises(CollectionNotFoundError):
        Collection(contrail_context, 'foo', fetch=True)

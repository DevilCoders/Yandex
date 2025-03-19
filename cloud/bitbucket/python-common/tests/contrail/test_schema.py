import pytest
from unittest import mock

import yc_common.clients.contrail.schema as schema
from yc_common.clients.contrail.context import Context
from yc_common.clients.contrail.resource import Resource, LinkedResources, LinkType, Collection

BASE_URL = "http://localhost:8082"


def test_load_non_existing_version():
    non_existing_version = "0"
    with pytest.raises(schema.SchemaVersionNotAvailable):
        schema.create_schema_from_version(non_existing_version)


def test_create_all_schema_versions():
    version_list = schema.list_available_schema_version()
    assert version_list

    for v in version_list:
        schema.create_schema_from_version(v)


@pytest.fixture(scope='module')
def schema_2_21():
    ctxt = Context()
    ctxt.schema = schema.create_schema_from_version('2.21')
    return ctxt


def test_attr_transformations(schema_2_21):
    lr = LinkedResources(schema_2_21, LinkType.REF,
                         Resource(schema_2_21, 'virtual-machine-interface', fq_name='foo'))
    assert lr._type_to_attr('virtual-machine') == 'virtual_machine_refs'
    assert lr._type_to_attr('virtual_machine') == 'virtual_machine_refs'
    assert lr._attr_to_type('virtual_machine') == 'virtual-machine'
    assert lr._attr_to_type('virtual_machine_refs') == 'virtual-machine'
    assert lr._attr_to_type('virtual_machine_back_refs') == 'virtual-machine-back'

    lr = LinkedResources(schema_2_21, LinkType.BACK_REF,
                         Resource(schema_2_21, 'virtual-machine-interface', fq_name='foo'))
    assert lr._type_to_attr('virtual-machine') == 'virtual_machine_back_refs'
    assert lr._attr_to_type('virtual_machine_back_refs') == 'virtual-machine'
    assert lr._attr_to_type('virtual_machine_refs') == 'virtual-machine-refs'

    lr = LinkedResources(schema_2_21, LinkType.CHILDREN,
                         Resource(schema_2_21, 'virtual-machine-interface', fq_name='foo'))
    assert lr._type_to_attr('virtual-machine') == 'virtual_machines'
    assert lr._attr_to_type('virtual_machines') == 'virtual-machine'
    assert lr._attr_to_type('virtual_machine_refs') == 'virtual-machine-ref'


def test_schema_refs(schema_2_21):
    schema_2_21.session = mock.MagicMock()
    schema_2_21.session.get_json.return_value = {
        "virtual-machine-interface": {
            "href": BASE_URL + "/virtual-machine-interface/ec1afeaa-8930-43b0-a60a-939f23a50724",
            "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
            "attr": None,
            "fq_name": [
                "virtual-machine-interface",
                "ec1afeaa-8930-43b0-a60a-939f23a50724"
            ],
            "bar_refs": [1, 2, 3],
            "virtual_machine_refs": [
                {
                    "href": BASE_URL + "/virtual-machine/15315402-8a21-4116-aeaa-b6a77dceb191",
                    "uuid": "15315402-8a21-4116-aeaa-b6a77dceb191",
                    "to": [
                        "bar",
                        "15315402-8a21-4116-aeaa-b6a77dceb191"
                    ]
                }
            ]
        }
    }
    vmi = Resource(schema_2_21, 'virtual-machine-interface', uuid='ec1afeaa-8930-43b0-a60a-939f23a50724', fetch=True)
    assert len(vmi.refs.virtual_machine) == 1
    assert type(vmi.refs.virtual_machine[0]) == Resource
    assert len(vmi.refs.bar) == 0
    assert [r.uuid for r in vmi.refs] == ['15315402-8a21-4116-aeaa-b6a77dceb191']


def test_schema_children(schema_2_21):
    schema_2_21.session = mock.MagicMock()
    schema_2_21.session.get_json.side_effect = [
        {
            "project": {
                "href": BASE_URL + "/project/ec1afeaa-8930-43b0-a60a-939f23a50724",
                "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
                "attr": None,
                "fq_name": [
                    "project",
                    "ec1afeaa-8930-43b0-a60a-939f23a50724"
                ],
                "virtual_networks": [
                    {
                        "href": BASE_URL + "/virtual-network/15315402-8a21-4116-aeaa-b6a77dceb191",
                        "uuid": "15315402-8a21-4116-aeaa-b6a77dceb191",
                        "to": [
                            "virtual-network",
                            "15315402-8a21-4116-aeaa-b6a77dceb191"
                        ]
                    }
                ]
            }
        },
        {
            'virtual-network': []
        }
    ]
    vmi = Resource(schema_2_21, 'project', uuid='ec1afeaa-8930-43b0-a60a-939f23a50724', fetch=True)
    assert len(vmi.children.virtual_network) == 1
    assert type(vmi.children.virtual_network) == Collection
    assert vmi.children.virtual_network.type, 'virtual-network'
    assert vmi.children.virtual_network.parent_uuid, vmi.uuid
    vmi.children.virtual_network.fetch()
    schema_2_21.session.get_json.assert_called_with(vmi.children.virtual_network.href, parent_id=vmi.uuid)


def test_schema_back_refs(schema_2_21):
    schema_2_21.session = mock.MagicMock()
    schema_2_21.session.get_json.side_effect = [
        {
            "virtual-network": {
                "href": BASE_URL + "/virtual-network/ec1afeaa-8930-43b0-a60a-939f23a50724",
                "uuid": "ec1afeaa-8930-43b0-a60a-939f23a50724",
                "attr": None,
                "fq_name": [
                    "virtual-network",
                    "ec1afeaa-8930-43b0-a60a-939f23a50724"
                ],
                "instance_ip_back_refs": [
                    {
                        "href": BASE_URL + "/instance-ip/15315402-8a21-4116-aeaa-b6a77dceb191",
                        "uuid": "15315402-8a21-4116-aeaa-b6a77dceb191",
                        "to": [
                            "instance-ip",
                            "15315402-8a21-4116-aeaa-b6a77dceb191"
                        ]
                    }
                ]
            }
        },
        {
            'instance-ip': []
        }
    ]
    vn = Resource(schema_2_21, 'virtual-network', uuid='ec1afeaa-8930-43b0-a60a-939f23a50724', fetch=True)
    assert len(vn.back_refs.instance_ip) == 1
    assert type(vn.back_refs.instance_ip) == Collection
    assert vn.back_refs.instance_ip.type
    assert vn.back_refs.instance_ip.back_refs_uuid
    vn.back_refs.instance_ip.fetch()
    schema_2_21.session.get_json.assert_called_with(vn.back_refs.instance_ip.href, back_ref_id=vn.uuid)

import json

from cloud.mdb.internal.python.racktables.client import (
    parse_racktables_project_id_networks_show_macro_response_json,
    RacktablesProjectIdNetworksShowMacroResponse,
    RacktablesProjectIdNetworksShowMacroResponseID,
)


def test_parse_racktables_project_id_networks_show_macro_response_json():
    json_data = json.loads(
        """{
    "ids": [
        {
            "id": "1589",
            "description": ""
        }
    ],
    "name": "_PGAASINTERNALNETS_",
    "owners": [
        "svc_hbf-agent_administration",
        "svc_internalmdb_development",
        "svc_internalmdb_hardware_management"
    ],
    "owner_service": "svc_internalmdb",
    "parent": null,
    "description": "QLOUD-934",
    "internet": 1,
    "secured": 0,
    "can_create_network": 1
}"""
    )
    parsed = parse_racktables_project_id_networks_show_macro_response_json(json_data)
    expected = RacktablesProjectIdNetworksShowMacroResponse(
        ids=[
            RacktablesProjectIdNetworksShowMacroResponseID(id='1589', description=''),
        ],
        name="_PGAASINTERNALNETS_",
        owners=["svc_hbf-agent_administration", "svc_internalmdb_development", "svc_internalmdb_hardware_management"],
        owner_service="svc_internalmdb",
        parent=None,
        description="QLOUD-934",
        internet=True,
        secured=False,
        can_create_network=True,
    )
    assert parsed == expected

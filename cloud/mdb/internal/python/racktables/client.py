import requests

from cloud.mdb.internal.python.racktables.errors import (
    RacktablesNotFoundError,
    RacktablesUnknownServerError,
    RacktablesUnknownClientError,
)

from typing import Optional, NamedTuple

from dbaas_common import tracing


class RacktablesClientConfig(NamedTuple):
    base_url: str
    oauth_token: str


class RacktablesProjectIdNetworksShowMacroResponseID(NamedTuple):
    id: str
    description: str


class RacktablesProjectIdNetworksShowMacroResponse(NamedTuple):
    ids: list[RacktablesProjectIdNetworksShowMacroResponseID]
    name: str
    owners: list[str]
    owner_service: str
    parent: Optional[str]
    description: str
    internet: bool
    secured: bool
    can_create_network: bool


class RacktablesClient:
    def __init__(
        self,
        config: RacktablesClientConfig,
    ) -> None:
        self.config = config

    @tracing.trace('Racktables project_id_networks show_macro')
    def show_macro(self, network_id: str) -> RacktablesProjectIdNetworksShowMacroResponse:
        url = f"{self.config.base_url}/export/project-id-networks.php?op=show_macro&name={network_id}"
        headers = {"Authorization": f"OAuth {self.config.oauth_token}"}
        resp = requests.get(url, headers=headers)

        if resp.status_code == 404:
            raise RacktablesNotFoundError

        if 400 <= resp.status_code < 500:
            raise RacktablesUnknownClientError

        elif 500 <= resp.status_code < 600:
            if resp.status_code == 500 and resp.text == f'Record \'macro\'#\'{network_id}\' does not exist':
                raise RacktablesNotFoundError
            raise RacktablesUnknownServerError

        json = resp.json()

        return parse_racktables_project_id_networks_show_macro_response_json(json)


def parse_racktables_project_id_networks_show_macro_response_json(
    json: dict,
) -> RacktablesProjectIdNetworksShowMacroResponse:
    return RacktablesProjectIdNetworksShowMacroResponse(
        ids=[
            RacktablesProjectIdNetworksShowMacroResponseID(id=x['id'], description=x['description'])
            for x in json['ids']
        ],
        name=json['name'],
        owners=json['owners'],
        owner_service=json['owner_service'],
        parent=json['parent'],
        description=json['description'],
        internet=json['internet'],
        secured=json['secured'],
        can_create_network=json['can_create_network'],
    )

import pathlib

import netifaces
from rich import box
from rich.table import Table, Column

from common.metadata import CloudMetadataClient
from tools.common import console

CLOUD_INIT_INSTANCE_ID_PATH = "/var/lib/cloud/data/instance-id"


def get_instance_id() -> str:
    # First, try to read instance_id from /var/lib/cloud/data/instance-id
    try:
        return pathlib.Path(CLOUD_INIT_INSTANCE_ID_PATH).read_text().strip()
    except FileNotFoundError:
        pass

    # If file doesn't exist, get instance/id from metadata service
    metadata_client = CloudMetadataClient()
    return metadata_client.get_metadata_value("id")


def print_network_interfaces():
    table = Table(
        Column("Interface", style="cyan3", header_style="bold cyan3"), "Addresses", "Network Mask", box=box.ROUNDED
    )
    for interface_name in netifaces.interfaces():
        addresses = netifaces.ifaddresses(interface_name)
        first_address = True
        for ipv4_address in addresses.get(netifaces.AF_INET, []):
            table.add_row(
                interface_name if first_address else "", ipv4_address.get("addr", "—"), ipv4_address.get("netmask", "—")
            )
            first_address = False
        for ipv6_address in addresses.get(netifaces.AF_INET6, []):
            table.add_row(
                interface_name if first_address else "", ipv6_address.get("addr", "—"), ipv6_address.get("netmask", "—")
            )
            first_address = False

    console.print(table)

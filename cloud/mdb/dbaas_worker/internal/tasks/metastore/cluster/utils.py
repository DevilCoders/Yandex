from typing import List

from lxml import etree  # type: ignore


def get_network_load_balancer_name(cid):
    return f'nlb-{cid}'


def get_target_group_name(cid):
    return f'target-group-{cid}'


def get_region_id() -> str:
    # TODO get region id from somewhere
    return 'ru-central1'


def get_security_group_ids() -> List[str]:
    # TODO get SGs from somewhere
    return ['c64flud531iut72r93b1']


def get_xml_config(postgres_cluster_hostname: str, db_name: str) -> str:
    metastore_site_config = {
        'metastore.thrift.uris': 'thrift://localhost:9083',
        'metastore.task.threads.always': 'org.apache.hadoop.hive.metastore.events.EventCleanerTask',
        'metastore.expression.proxy': 'org.apache.hadoop.hive.metastore.DefaultPartitionExpressionProxy',
        'javax.jdo.option.ConnectionDriverName': 'org.postgresql.Driver',
        'javax.jdo.option.ConnectionURL': f'jdbc:postgresql://{postgres_cluster_hostname}:6432/{db_name}?ssl=true',
        'datanucleus.autoCreateSchema': False,
    }

    root = etree.Element('configuration')
    for key, value in metastore_site_config.items():
        property_element = etree.Element('property')
        name_element = etree.Element('name')
        name_element.text = key
        property_element.append(name_element)

        value_element = etree.Element('value')
        if not isinstance(value, str):
            value = str(value).lower()
        value_element.text = value
        property_element.append(value_element)

        root.append(property_element)

    parts = [
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
        '<?xml-stylesheet type="text/xsl" href="configuration.xsl"?>',
        etree.tostring(root, pretty_print=True).decode(),
    ]
    return '\n'.join(parts)

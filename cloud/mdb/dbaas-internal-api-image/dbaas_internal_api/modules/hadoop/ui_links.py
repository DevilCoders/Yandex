# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster ui_links methods
"""

from flask import current_app
from typing import Dict

from ...utils.feature_flags import ensure_feature_flag
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import ClusterService


@register_request_handler(MY_CLUSTER_TYPE, Resource.HADOOP_UI_LINK, DbaasOperation.LIST)
def list_ui_links_handler(cluster: Dict, **_) -> Dict:
    """
    List ui links handler
    """
    ensure_feature_flag('MDB_DATAPROC_UI_PROXY')
    pillar = get_cluster_pillar(cluster)
    links = []
    if pillar.ui_proxy:
        ui_links_config = current_app.config['HADOOP_UI_LINKS']
        service_names = ClusterService.to_strings(pillar.services)
        if pillar.bypass_knox:
            master_fqdn = pillar.subcluster_main['hosts'][0]
            master_subdomain = master_fqdn.split('.')[0]
            for service_name in service_names:
                service_cfg = ui_links_config.get('services', {}).get(service_name, [])
                for ui in service_cfg:
                    url = (
                        ui_links_config['knoxless_base_url']
                        .replace('${CLUSTER_ID}', cluster['cid'])
                        .replace('${HOST}', master_subdomain)
                        .replace('${PORT}', str(ui['port']))
                    )
                    links.append({'name': ui['name'], 'url': url})
        else:
            base_url = ui_links_config['base_url'].replace('${CLUSTER_ID}', cluster['cid'])
            for service_name in service_names:
                service_cfg = ui_links_config.get('services', {}).get(service_name, [])
                for ui in service_cfg:
                    url = base_url + ui['code'] + '/'
                    links.append({'name': ui['name'], 'url': url})
    return {'links': links}

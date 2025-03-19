#!/usr/bin/env python3
from datetime import datetime
import yaml
from copy import deepcopy
from yc_solomon_cli.client import SolomonClient, SolomonClientError

base_solomon_cli_alert = {
    'menu': 'SLO Alerts',
    'menu_type': 'admin',
    'api_path': '/alerts/',
    'tags': ["slo"],
    'entities': {}
}

cfgs = {
    'cpu_util_alert': 'alerts/slo/sli_cpu_util_per_instance_id.j2',
    'arp_replies': 'alerts/slo/sli_arp_replies.j2',
    'instance_health': 'alerts/slo/sli_instance_health.j2',
    'e2e': 'alerts/slo/sli_e2e.j2',
    'meta': 'alerts/slo/sli_meta.j2',
    'slo_vm': 'alerts/slo/slo_vm.j2'
}

CLIENT_CLOUD_IDS = {
    'ozon': 'b1gu1lrtdq8s1svvmqso',
    'decathlon': 'b1ggien8hdc5b5t22k9i',
    'edadeal': 'b1g3upkn63ta2mo9o6lh'
}


def main():
    client = SolomonClient('https://solomon.yandex.net/api/v2/projects/yandexcloud', dry_run=False)
    for org, cloud_id in CLIENT_CLOUD_IDS.items():
        org_alerts = deepcopy(base_solomon_cli_alert)
        org_alerts['tags'].append('slo-{}'.format(org))
        host_pair = {}
        for dc in ['sas', 'myt', 'vla']:
            host_pair[dc] = {}
            query = {
                'cluster': 'cloud_prod_compute_%s' % dc,
                'service': 'internals',
                'service_name': '*',
                'source': '*',
                'instance_id': '*',
                'metric': 'instance_health',
                'cloud_id': '%s' % cloud_id,
                'host!': 'cluster'
            }
            try:
                sensors = client.get_sensors(query, pagination=True, pageSize=300, raw=False)
            except SolomonClientError as e:
                print("Error while getting info for query %s %s %s" % query, e, cloud_id)
            else:
                if sensors:
                    for sensor in sensors:
                        host = sensor['labels']['host']
                        line_created = datetime.strptime(sensor['createdAt'], '%Y-%m-%dT%H:%M:%SZ')
                        vm_id = sensor['labels']['instance_id']
                        if vm_id not in host_pair[dc]:
                            host_pair[dc][vm_id] = {
                                'host': host,
                                'createdAt': line_created
                            }
                        elif host_pair[dc][vm_id]['createdAt'] < line_created:
                            host_pair[dc][vm_id] = {
                                'host': host,
                                'createdAt': line_created
                            }
        for dc, vms in host_pair.items():
            for vm_id, vm_info in vms.items():
                host= vm_info['host']
                for name, tmpl in cfgs.items():
                    context = {
                        'host': host,
                        'dc': dc,
                        'cloud_id': cloud_id,
                        'org': org,
                        'notificationChannels': [],
                    }
                    if name in ['e2e', 'nginx_metadata']:
                        alert = 'slo_{}_{}'.format(host, name)
                    else:
                        alert = 'slo_{}_{}_{}'.format(org, name, vm_id)
                        context['org'] = org
                        context['vm_id'] = vm_id
                    if name == 'slo_vm':
                        context['notificationChannels'] = ["slo_per_vm_email"]
                    org_alerts['entities'][alert] = {
                        'template': tmpl,
                        'context': context,
                    }
        with open('./{}_config.yaml.next'.format(org), 'w') as f:
            yaml.safe_dump(org_alerts, f, indent=2)

if __name__ == '__main__':
    main()

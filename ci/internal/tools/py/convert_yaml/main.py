import argparse
import ruamel.yaml


def main():
    parser = argparse.ArgumentParser(description='Add HDD disk')
    parser.add_argument('-i', required=True)
    parser.add_argument('-o', required=True)
    args = parser.parse_args()

    file_in = args.i
    file_out = args.o
    print("Loading %s" % file_in)

    yaml = ruamel.yaml.YAML()  # defaults to round-trip if no parameters given
    with open(file_in) as f:
        root = yaml.load(f)

    spec = root['spec']
    spec['revision_info']['description'] = 'Add disk_hdd'

    units = spec['deploy_units']
    for key in units:
        unit = units[key]

        pod = unit['multi_cluster_replica_set']['replica_set']['pod_template_spec']['spec']
        disks = pod['disk_volume_requests']

        assert len(disks) in (1, 2), "Expect 1 or 2 disks, found %s" % len(disks)

        hdd_quota_policy = {
            'capacity': 107374182400,  # 100 GiB
            'bandwidth_guarantee': 5242880,  # 5 MiB/s
            'bandwidth_limit': 20971520  # 20 MiB/s
        }
        for disk in disks:
            storage_class = disk['storage_class']
            if storage_class == 'ssd':
                disk['id'] = 'disk_ssd'
                disk['labels'] = {'used_by_infra': True, 'volume_type': 'root_fs', 'mount_path': '/'}
            elif storage_class == 'hdd':
                disk['id'] = 'disk_hdd'
                disk['quota_policy'] = hdd_quota_policy

        if len(disks) == 1:
            disks.append({
                'id': 'disk_hdd',
                'quota_policy': hdd_quota_policy,
                'storage_class': 'hdd'
            })

        pod_agent = pod['pod_agent_payload']
        pod_agent['meta'] = {'sidecar_volume': {'storage_class': 'ssd'}}

        pod_spec = pod_agent['spec']
        for box in pod_spec['boxes']:
            box['virtual_disk_id_ref'] = 'disk_ssd'
            box['volumes'] = [
                {'mode': 'read_write', 'mount_point': '/logs', 'volume_ref': 'logs'},
                {'mode': 'read_write', 'mount_point': '/hprof', 'volume_ref': 'hprof'}
            ]

        for layer in pod_spec['resources']['layers']:
            layer['virtual_disk_id_ref'] = 'disk_ssd'

        pod_spec['volumes'] = [
            {'generic': {}, 'id': 'logs', 'virtual_disk_id_ref': 'disk_ssd'},
            {'generic': {}, 'id': 'hprof', 'virtual_disk_id_ref': 'disk_hdd'}
        ]

        unit['logbroker_config'] = {
            'logs_virtual_disk_id_ref': 'disk_ssd',
            'sidecar_volume': {
                'storage_class': 'ssd'
            }
        }

    print("Saving %s" % file_out)
    with open(file_out, 'w') as f:
        yaml.dump(root, f)


if __name__ == "__main__":
    main()

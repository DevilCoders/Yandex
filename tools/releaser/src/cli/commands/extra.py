import click

from library.python.vault_client.instances import Production as VaultClient

from tools.releaser.src.cli import options, utils
from tools.releaser.src.lib.awacs import AwacsClient, AwacsHttpClient
from tools.releaser.src.lib.nanny import NannyClient


@click.command(help='add push-client to l7 balancers in awacs namespace')
@options.domain_type_option
@options.tvm_client_id_option
@options.logroker_topic_option
@options.dry_run_option
def add_logs(domain_type, tvm_client_id, logbroker_topic, dry_run):
    utils.maybe_download_certificate(dry_run=dry_run)

    yav_client = VaultClient(decode_files=True)
    tvm_secret_name = f'tvm.secret.{tvm_client_id}'
    tvm_secrets = yav_client.list_secrets(query=tvm_secret_name, query_type='exact')
    if not tvm_secrets:
        click.echo(f'Can\'t find secret for tvm with name {tvm_secret_name}. Check tvm_client_id or permissions.')
        return
    tvm_secret = tvm_secrets[0]

    awacs_client = AwacsClient(domain_type)
    awacs_http_client = AwacsHttpClient()
    balancers = awacs_client.get_balancers()
    if not balancers:
        click.echo(f'Can\'t find balancers for awacs namespace {domain_type}.')
        return

    for balancer in balancers:
        service = balancer.spec.config_transport.nanny_static_file.service_id
        click.echo(f'Updating {balancer.meta.id} balancer / {service} nanny service')

        nanny_client = NannyClient(service)
        runtime_attrs = nanny_client.get_runtime_attrs()

        has_changes = False

        sandbox_files = runtime_attrs['content']['resources']['sandbox_files']
        has_changes |= add_or_update_resource(sandbox_files, push_client_spec, 'local_path')

        static_files = runtime_attrs['content']['resources']['static_files']
        push_client_config_spec['content'] = push_client_config_spec['content'].format(
            tvm_client_id=tvm_client_id, logbroker_topic=logbroker_topic,
        )
        has_changes |= add_or_update_resource(static_files, push_client_config_spec, 'local_path')

        volumes = runtime_attrs['content']['instance_spec']['volume']

        def fill_secret(spec):
            token, _ = yav_client.create_token(tvm_secret['uuid'], 2002924, service)
            spec['vaultSecretVolume'] = {
                'vaultSecret': {
                    'secretVer': tvm_secret['last_secret_version']['version'],
                    'secretId': tvm_secret['uuid'],
                    'delegationToken': token,
                    'secretName': tvm_secret_name,
                }
            }

        updated = False
        for i, volume in enumerate(volumes):
            if volume['name'] == push_client_volume_spec['name']:
                updated = True
                secret = volume['vaultSecretVolume']['vaultSecret']
                if secret['secretId'] != tvm_secret['uuid'] or secret['secretVer'] != tvm_secret['last_secret_version']['version']:
                    fill_secret(push_client_volume_spec)
                    volumes[i] = push_client_volume_spec
                    has_changes = True
                    break

        if not updated:
            fill_secret(push_client_volume_spec)
            volumes.append(push_client_volume_spec)
            has_changes = True

        if has_changes and not dry_run:
            nanny_client.update_runtime_attrs(runtime_attrs)
            click.echo(click.style(
                f'Updated balancer spec, now you can check diff if you want: '
                f'https://nanny.yandex-team.ru/ui/#/services/catalog/{service}/ '
                f'and when resume config update to apply it on '
                f'https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/{domain_type}/show/',
                fg='green',
            ))
        elif dry_run:
            click.echo(click.style('[DRY RUN] Updated balancer spec', fg='green'))

        if not dry_run:
            awacs_http_client.enable_pushclient(
                balancer_id=balancer.meta.id,
                namespace_id=balancer.meta.namespace_id,
                version=balancer.meta.version,
            )


def add_or_update_resource(resources, config, key):
    for i, resource in enumerate(resources):
        if resource[key] == config[key]:
            if resource != config:
                resources[i] = config
                click.echo(f'Update {resource[key]} resource')
                return True
            else:
                click.echo(f'Skip {resource[key]} resource')
                return False

    click.echo(f'Add {config[key]} resource')
    resources.append(config)
    return True


push_client_spec = {
    'task_type': 'BUILD_STATBOX_PUSHCLIENT',
    'task_id': '764211995',
    'resource_id': '1701136678',
    'extract_path': '',
    'is_dynamic': False,
    'local_path': 'push-client',
    'resource_type': 'STATBOX_PUSHCLIENT'
}


push_client_config_spec = {
    'is_dynamic': False,
    'content': '''watcher:
    state: /logs/pushclient
    drop_on_error: 1
network:
    master-addr: "logbroker.yandex.net"
    proto: pq
    transport: ipv6
    tvm-client-id: {tvm_client_id}
    tvm-server-id: 2001059
    tvm-secret-file: pushclient-secrets/client_secret
logger:
    file: /logs/current-pushclient
    remote: 0
    telemetry_interval: -1
    level: 6
topic: {logbroker_topic}
files:
  - name: /logs/current-access_log-balancer-80
    sid: [host, name, lines: 1]
    send_delay: 5
  - name: /logs/current-access_log-balancer-443
    sid: [host, name, lines: 1]
    send_delay: 5''',
    'local_path': 'push-client_real.conf'
}

push_client_volume_spec = {
    'name': 'pushclient-secrets',
    'version': '',
    'secretVolume': {
        'keychainSecret': {
            'keychainId': '',
            'secretId': '',
            'secretRevisionId': ''
        },
        'secretName': ''
    },
    'templateVolume': {
        "template": []
    },
    'type': 'VAULT_SECRET',
    'itsVolume': {
        'maxRetryPeriodSeconds': 300,
        'periodSeconds': 60,
        'itsUrl': 'http://its.yandex-team.ru/v1'
    }
}

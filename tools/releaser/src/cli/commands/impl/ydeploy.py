import sys

import click

from gitchronicler import chronicler

from tools.releaser.src.lib import ydeploy, awacs
from tools.releaser.src.cli import utils


def parse_deploy_units(deploy_units, default_box=None):
    # TODO: do this in the command parsing
    units = [
        # `unit` or `unit:box`
        deploy_unit.split(':', 1)
        for deploy_unit in deploy_units
    ]
    # Using the unit name as box name if it was not specified in any other way.
    units = [
        (
            (unit[0], default_box if default_box is not None else unit[0])
            if len(unit) == 1
            else unit
        )
        for unit in units
    ]
    return units


def _list_hosts(stage, deploy_units=None, for_ssh=True):
    deploy_client = ydeploy.DeployClient(stage)
    deploy_boxes = deploy_client.list_hosts()
    if deploy_units:
        deploy_units_spec = parse_deploy_units(deploy_units)
        deploy_units_set = set(
            deploy_unit
            for deploy_unit, _ in deploy_units_spec)
        deploy_boxes = [
            deploy_box
            for deploy_box in deploy_boxes
            if deploy_box['deploy_unit'] in deploy_units_set
        ]

    if not deploy_boxes:
        utils.panic('No matching instances found')

    if for_ssh:
        hostnames = [deploy_box['url'] for deploy_box in deploy_boxes]
    else:
        hostnames = [deploy_box['fqdn'] for deploy_box in deploy_boxes]
    return hostnames


def hosts(stage, deploy_units=None, for_ssh=True):
    hostlist = _list_hosts(stage=stage, deploy_units=deploy_units, for_ssh=for_ssh)
    # Not using `click.echo` because this is an output for use in scripts, not
    # for reading by the user.
    sys.stdout.write(''.join('{}\n'.format(host) for host in hostlist))


def ssh(stage, deploy_units, shellwrap, dry_run, cmd_args, command):
    hostlist = _list_hosts(stage=stage, deploy_units=deploy_units, for_ssh=True)
    return utils.ssh_to_random_host(
        host_urls=hostlist, shellwrap=shellwrap, dry_run=dry_run,
        cmd_args=cmd_args, command=command)


def pssh(stage, deploy_units, shellwrap, dry_run, pssh_cmd, cmd_args):
    hostlist = _list_hosts(stage=stage, deploy_units=deploy_units, for_ssh=True)
    return utils.pssh(
        host_urls=hostlist, shellwrap=shellwrap, dry_run=dry_run,
        pssh_cmd=pssh_cmd, cmd_args=cmd_args)


def deploy(image, version, stage, deploy_units, box, dump, deploy_comment_format, from_version, dry_run, draft):
    """
    Как быстрое решение для различного именования боксов добавляем
    формат deploy_unit:box, deploy_unit_2:box_2 ...

    При этом для тех у кого не указано, будет использоваться обычный box
    """
    deploy_client = ydeploy.DeployClient(stage, dump)
    deploy_units_spec = parse_deploy_units(deploy_units, default_box=box)
    for deploy_unit, box in deploy_units_spec:
        deploy_client.update_image(
            deploy_unit=deploy_unit,
            box=box,
            image=image,
            tag=version,
        )

    deploy_comment = utils.get_deploy_comment(
        deploy_comment_format,
        changelog_records=chronicler.get_changelog_records(),
        from_version=from_version,
        new_version=version)

    deploy_client.add_comment(deploy_comment)

    if not dry_run:
        deploy_client.deploy(draft=draft)


def env_dump(stage):
    return ydeploy.DeployClient(stage).dump()


def env_delete(stage):
    return ydeploy.DeployClient(stage).delete()


def add_domain(stage, domain, namespace, dry_run):
    if dry_run:
        click.echo(f'[DRY RUN] [awacs] created domain with id `{domain}`')
        return

    deploy_client = ydeploy.DeployClient(stage)

    awacs_client = awacs.AwacsClient(namespace)

    default_domain = awacs_client.get_domain('default-domain')
    if default_domain is None:
        click.echo(f'[awacs] domain with id `default-domain` (namespace={namespace}) doesn\'t exists', err=True)
        return

    upstream_ids = default_domain.spec.yandex_balancer.config.include_upstreams.ids
    upstreams = awacs_client.get_upstreams(upstream_ids)

    new_upstreams = []
    for upstream in upstreams:
        backend_ids = upstream.spec.yandex_balancer.config.l7_upstream_macro.flat_scheme.backend_ids
        backends = awacs_client.get_backends(backend_ids)

        deploy_units = set()
        for backend in backends:
            for endpoint_set in backend.spec.selector.yp_endpoint_sets:
                deploy_units.add(endpoint_set.endpoint_set_id.split('.')[1])

        deploy_units_for_new_backend = []
        for deploy_unit in deploy_units:
            du = deploy_client.stage_data['spec']['deploy_units'].get(deploy_unit)
            if du:
                if 'multi_cluster_replica_set' in du:
                    for cluster in du['multi_cluster_replica_set']['replica_set']['clusters']:
                        deploy_units_for_new_backend.append((deploy_unit, cluster['cluster']))
                elif 'replica_set' in du:
                    for cluster in du['replica_set']['per_cluster_settings'].keys():
                        deploy_units_for_new_backend.append((deploy_unit, cluster))

        if deploy_units_for_new_backend:
            new_backend = awacs_client.copy_backend(backend, stage, deploy_units_for_new_backend)
            click.echo(f"[awacs] created backend with id `{new_backend.meta.id}`")
            new_upstream = awacs_client.copy_upstream(upstream, stage, new_backend.meta.id)
            click.echo(f"[awacs] created upstream with id `{new_upstream.meta.id}`")
            new_upstreams.append(new_upstream)

    existing_domain = awacs_client.get_domain(domain)
    if existing_domain is not None:
        click.echo(f'[awacs] domain {domain} (namespace={namespace}) already exists', err=True)
        return

    new_domain = awacs_client.copy_domain(default_domain, domain, new_upstreams)
    click.echo(f"[awacs] created domain with id `{new_domain.meta.id}`")

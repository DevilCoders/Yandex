"""
Steps related to backup service
"""

from behave import register_type, when
from parse_type import TypeBuilder

from tests.helpers import docker

register_type(BackupServiceUsage=TypeBuilder.make_enum({
    'enable': True,
    'disable': False,
}))


def set_backup_service_usage(context, cid, usage):
    """
    Switches backup service usage
    """

    backup_service_container = docker.get_container(context, 'backup01')

    cmd = '/mdb-backup-cli --config-path / backup_service_usage --cluster-id={cid} --enabled={usage}'.format(
        cid=cid, usage=usage)
    code, output = backup_service_container.exec_run(cmd)
    if code != 0:
        raise RuntimeError('{cmd} failed with {code}: {output}'.format(cmd=cmd, code=code, output=output.decode()))

    cmd = '/mdb-backup-cli --config-path / roll_metadata --cluster-id={cid}'.format(cid=cid)
    code, output = backup_service_container.exec_run(cmd)
    if code != 0:
        raise RuntimeError('{cmd} failed with {code}: {output}'.format(cmd=cmd, code=code, output=output.decode()))


def import_backups(context, cid, args):
    """
    Imports backups from storage
    """

    backup_service_container = docker.get_container(context, 'backup01')
    cmd = '/mdb-backup-cli --config-path / import_backups --cluster-id={cid} {args}'.format(
        cid=cid, args=" ".join(args))
    code, output = backup_service_container.exec_run(cmd)
    if code != 0:
        raise RuntimeError('{cmd} failed with {code}: {output}'.format(cmd=cmd, code=code, output=output.decode()))


@when('we {usage:BackupServiceUsage} backup service')
def step_set_backup_service(context, usage):
    set_backup_service_usage(context, cid=context.cluster['id'], usage=usage)


@when('we import cluster backups to backup service')
@when('we import cluster backups to backup service with args "{args}"')
def import_backups_from_storage(context, args=""):
    """
    Backup imports step
    """

    import_backups(context, cid=context.cluster['id'], args=args.split())

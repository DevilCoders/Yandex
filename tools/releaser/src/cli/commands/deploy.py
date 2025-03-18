import os
import codecs
import socket

import click

from gitchronicler import chronicler

from tools.releaser.src.cli import options, arguments, utils
from tools.releaser.src.cli.commands.impl import ydeploy, qloud
from tools.releaser.src.lib.vcs import get_vcs_ctl


@click.command(help='add domain to environment')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.domain_option
@options.domain_type_option
@options.dry_run_option
def add_domain(qloudinst, deploy, project, application, environment, domain, domain_type, dry_run):
    utils.maybe_download_certificate(dry_run=dry_run)

    try:
        socket.getaddrinfo(domain, None)
    except socket.gaierror:
        if not click.confirm('No DNS for specified domain "%s", are you sure to continue?' % domain):
            return

    if deploy:
        ydeploy.add_domain(environment, domain, domain_type, dry_run)
    else:
        qloud.add_domain(qloudinst, project, application, environment, domain, domain_type, dry_run)


@click.command(help='add deploy hook to environment')
@options.qloud_instance_option
@options.project_option
@options.application_option
@options.environment_option
@options.deploy_hook_option
@options.dry_run_option
def add_deploy_hook(qloudinst, project, application, environment, deploy_hook, dry_run):
    return qloud.add_deploy_hook(qloudinst, project, application, environment, deploy_hook, dry_run)


@click.command(help='display status of instances')
@options.qloud_instance_option
@options.project_option
@options.applications_option
@options.environment_option
@options.dry_run_option
def status(qloudinst, project, applications, environment, dry_run):
    qloud.status(qloudinst, project, applications, environment, dry_run)


@click.command(help='deploy components using image')
@options.image_option
@options.version_option
@options.from_version_option
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.applications_option
@options.environment_option
@options.environment_target_state_option
@options.deploy_draft_option
@options.components_option
@options.box_option
@options.environment_dump_from_option
@options.deploy_comment_format_option
@options.pull_vcs_option
@options.dry_run_option
@options.env_vars_option
def deploy(image, version, from_version, qloudinst, deploy,
           project, applications, environment, components, box, deploy_draft,
           deploy_comment_format, target_state, dump, pull_vcs, dry_run, var):

    version = version or chronicler.get_current_version()
    utils.maybe_download_certificate(dry_run=dry_run)

    if pull_vcs:
        get_vcs_ctl(dry_run=dry_run).pull()

    if dump:
        if not os.path.exists(dump):
            raise RuntimeError('Файл {} не найден'.format(dump))

    if deploy or box:
        ydeploy.deploy(
            image=image,
            version=version,
            stage=environment,
            deploy_comment_format=deploy_comment_format,
            from_version=from_version,
            deploy_units=components,
            box=box,
            dump=dump,
            dry_run=dry_run,
            draft=deploy_draft,
        )
    else:
        qloud.deploy(
            image=image,
            version=version,
            qloudinst=qloudinst,
            project=project,
            applications=applications,
            environment=environment,
            components=components,
            deploy_comment_format=deploy_comment_format,
            target_state=target_state,
            dump=dump,
            dry_run=dry_run,
            var=var,
            from_version=from_version,
        )


@click.command(help='delete environment')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.non_interactive_option
@options.dry_run_option
def env_delete(qloudinst, deploy, project, application, environment, non_interactive, dry_run):
    if dry_run:
        click.echo(f'[DRY RUN] Deleting environment {environment}')
        return

    utils.maybe_download_certificate(dry_run=dry_run)

    if deploy:
        ydeploy.env_delete(environment)
    else:
        qloud.env_delete(qloudinst, project, application, environment, non_interactive)


@click.command(help='dump environment to file')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.environment_dump_to_option
@options.dry_run_option
def env_dump(qloudinst, deploy, project, application, environment, dump, dry_run):
    utils.maybe_download_certificate(dry_run=dry_run)

    if deploy:
        env_dump = ydeploy.env_dump(environment)
    else:
        env_dump = qloud.env_dump(qloudinst, project, application, environment)

    if dry_run:
        click.echo(f'[DRY RUN] Writing environment dump in {dump}\n{env_dump}')
    else:
        with codecs.open(dump, 'w', 'utf-8') as f:
            f.write(env_dump)


@click.command(help='list hostnames of a component to stdout')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.components_option
def hosts(qloudinst, deploy, project, application, environment, components):
    if deploy:
        ydeploy.hosts(environment, components)
    else:
        qloud.hosts(qloudinst, project, application, environment, components)


@click.command(help='ssh to a randomly picked instance')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.components_option
@options.cmd_shell_wrap_command
@options.dry_run_option
@options.ssh_command
@arguments.optional_subcommand_arguments
def ssh(qloudinst, deploy, project, application, environment, components, shellwrap, dry_run, cmd_args, command):
    if deploy:
        ydeploy.ssh(
            environment, components,
            shellwrap=shellwrap, dry_run=dry_run, cmd_args=cmd_args, command=command)
    else:
        qloud.ssh(qloudinst, project, application, environment, components, shellwrap, dry_run, cmd_args, command)


@click.command(help='pssh to random instance of component')
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.application_option
@options.environment_option
@options.components_option
@options.pssh_cmd
@options.cmd_shell_wrap_command
@options.dry_run_option
@arguments.optional_subcommand_arguments
def pssh(qloudinst, deploy, project, application, environment, components, shellwrap, dry_run, pssh_cmd, cmd_args):
    if deploy:
        ydeploy.pssh(
            environment, components,
            shellwrap=shellwrap, dry_run=dry_run, pssh_cmd=pssh_cmd, cmd_args=cmd_args)
    else:
        qloud.pssh(qloudinst, project, application, environment, components, shellwrap, dry_run, pssh_cmd, cmd_args)

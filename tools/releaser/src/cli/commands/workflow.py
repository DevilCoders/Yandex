import sh
import uuid
import traceback

import click

from tools.releaser.src.conf import cfg
from tools.releaser.src.cli import options, utils


def get_commands():
    from tools.releaser.src.cli import main
    result = main.cli.commands
    # This is necessary to support python-click < 7.0, where the commands are
    # represented over '_' e.g. 'add_domain'.
    # If the support for click < 7.0 is ever dropped, this line can be removed.
    result = {key.replace('_', '-'): val for key, val in result.items()}
    return result


def call_command(name, *args, **kwargs):
    command = get_commands().get(name)
    if command is None:
        raise RuntimeError('Unknown command %s' % name)
    return command.callback(*args, **kwargs)


class Step(object):

    def __init__(self, name, **kwargs):
        self.name = name
        self.kwargs = kwargs

    def execute(self, **extra_params):
        params = dict(self.kwargs, **extra_params)
        call_command(
            name=self.name,
            **params
        )


class SpecialStep(Step):

    def execute(self, **extra_params):
        raise Exception("Not supposed to be executed")


class NoRollbackMarker(SpecialStep):
    """
    Метка, обозначающая, что после этого псевдошага не нужно
    делать роллбэк предыдущих команд.
    """

    def __init__(self, name='__no_rollback__', **kwargs):
        super(NoRollbackMarker, self).__init__(name, **kwargs)


def run_workflow(cmd, steps, skip=None, dry_run=None, no_rollback=None):
    if skip is None:
        skip = cfg.get_command_option(cmd, 'skip') or ''
    skip = skip.split(',') if skip else []

    if dry_run is None:
        dry_run = cfg.get_command_option(cmd, 'dry_run') or False

    if no_rollback is None:
        no_rollback = cfg.get_command_option(cmd, 'no_rollback') or False

    executed_steps = []
    rollback_steps = []
    do_rollback_on_fail = not no_rollback

    known_commands = set(get_commands())
    steps_commands = set(
        step.name for step in steps
        if not isinstance(step, SpecialStep))
    unknown_steps_commands = steps_commands - known_commands
    if unknown_steps_commands:
        raise RuntimeError("Unknown command(s): {}".format(
            ", ".join(sorted(unknown_steps_commands))))

    for step_num, step in enumerate(steps, start=1):
        if isinstance(step, NoRollbackMarker):
            utils.echo_workflow_step('No rollback from this point', step_num)
            do_rollback_on_fail = False
            continue

        if step.name in skip:
            utils.echo_workflow_step('Skipping ' + step.name, step_num)
            continue

        echo_step(step, dry_run=dry_run, step_num=step_num)
        try:
            step.execute(dry_run=dry_run)
        except Exception:
            traceback.print_exc()
            click.echo('Step %s %s failed. Aborting' % (step_num, step.name))
            if do_rollback_on_fail:
                rollback_steps = reversed(executed_steps)
            else:
                raise click.ClickException('No rollback, exit')
            break
        else:
            executed_steps.append(step)

    if rollback_steps:
        click.echo('Rollback previous steps')
        for step in rollback_steps:
            click.echo('Rollback step %s' % step.name)
            rollback_step_name = 'rollback-' + step.name
            if rollback_step_name not in get_commands():
                click.echo('Command ' + rollback_step_name + ' not found')
                continue
            rollback_step = Step(rollback_step_name, **step.kwargs)
            echo_step(rollback_step, dry_run=dry_run, step_num='x')
        raise click.ClickException('Rollback finished')


def echo_step(step, dry_run, step_num='*'):
    step_num = str(step_num)
    msg = 'Executing'
    if dry_run:
        msg += ' (dry run)'

    # TODO: тут хорошо бы рендерить строку вызова команды именно такую,
    # которую можно запустить отдельно с длинными параметрами,
    # но я пока не думал как это проще сделать
    # (может в click.Command есть все нужное для этого)
    msg = ' '.join([
        msg,
        step.name,
        ' '.join([
            '--%s=%s' % (key, value)
            for key, value in step.kwargs.items()
            if value is not None
        ]),
    ])
    utils.echo_workflow_step(msg, step_num)


@click.command(
    help='[WORKFLOW] Update changelog, build, '
         'push registry and vcs repo and deploy'
)
@options.workflow_skip_option
@options.workflow_no_rollback_option
@options.dry_run_option
@options.git_remote_option
@options.direct_push_option
@options.version_option
@options.from_version_option
@options.version_file_path
@options.version_build_arg_option
@options.non_interactive_option
@options.image_option
@options.buildfile_option
@options.sandbox_option
@options.dockerfile_option
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
@options.pull_base_image_option
@options.pull_vcs_option
@options.env_vars_option
def release(
        skip,
        no_rollback,
        dry_run,
        remote,
        direct_push,
        version,
        version_file_path,
        version_build_arg,
        non_interactive,
        image,
        buildfile,
        sandbox,
        dockerfile,
        qloudinst,
        deploy,
        project,
        applications,
        environment,
        target_state,
        components,
        box,
        deploy_draft,
        dump,
        deploy_comment_format,
        pull_base_image,
        pull_vcs,
        from_version=None,
        var=None,
):
    steps = [
        Step('changelog', non_interactive=non_interactive, version=version, pull_vcs=pull_vcs),
        Step('version', version=version, version_file_path=version_file_path),
        Step('vcs-commit', version=version, direct_push=direct_push),
        Step('vcs-tag', version=version),
        Step(
            'build',
            image=image,
            buildfile=buildfile,
            dockerfile=dockerfile,
            sandbox=sandbox,
            version=version,
            version_build_arg=version_build_arg,
            pull_base_image=pull_base_image,
        ),
        NoRollbackMarker(),
        Step('push', sandbox=sandbox, image=image, version=version),
        Step('vcs-push', remote=remote, direct_push=direct_push),
        Step(
            'deploy',
            image=image,
            version=version,
            from_version=from_version,
            qloudinst=qloudinst,
            deploy=deploy,
            project=project,
            applications=applications,
            environment=environment,
            components=components,
            box=box,
            deploy_comment_format=deploy_comment_format,
            target_state=target_state,
            dump=dump,
            deploy_draft=deploy_draft,
            # Здесь сбрасываем флаг pull_vcs, чтобы в ходе составной операции не получить разный код в шагах.
            pull_vcs=False,
            var=var or None,
        ),
    ]
    run_workflow(
        cmd='release',
        steps=steps,
        skip=skip,
        dry_run=dry_run,
        no_rollback=no_rollback,
    )


@click.command(
    help='[WORKFLOW] Update changelog and push it to vcs repo'
)
@options.workflow_skip_option
@options.workflow_no_rollback_option
@options.dry_run_option
@options.git_remote_option
@options.direct_push_option
@options.version_option
@options.version_file_path
@options.non_interactive_option
@options.pull_vcs_option
def release_changelog(
        skip,
        no_rollback,
        dry_run,
        remote,
        direct_push,
        version,
        version_file_path,
        non_interactive,
        pull_vcs,
):
    steps = [
        Step('changelog', non_interactive=non_interactive, version=version, pull_vcs=pull_vcs),
        Step('version', version=version, version_file_path=version_file_path),
        Step('vcs-commit', version=version, direct_push=direct_push),
        Step('vcs-tag', version=version),
        Step('vcs-push', remote=remote, direct_push=direct_push),
    ]
    run_workflow(
        cmd='release-changelog',
        steps=steps,
        skip=skip,
        dry_run=dry_run,
        no_rollback=no_rollback,
    )


@click.command(
    help='[WORKFLOW] build stand'
)
@options.workflow_skip_option
@options.workflow_no_rollback_option
@options.dry_run_option
@options.stand_option
@options.version_build_arg_option
@options.image_option
@options.buildfile_option
@options.sandbox_option
@options.dockerfile_option
@options.qloud_instance_option
@options.deploy_option
@options.project_option
@options.applications_option
@options.components_option
@options.box_option
@options.environment_dump_from_option
@options.ticket
@options.get_ticket_from_stand_name
@options.get_ticket_from_last_commit_message
@options.get_ticket_from_current_branch_name
@options.domain_tpl_option
@options.domain_type_option
@options.deploy_hook_option
@options.pull_base_image_option
@options.env_vars_option
def stand(
        skip,
        no_rollback,
        dry_run,
        standname,
        version_build_arg,
        image,
        buildfile,
        sandbox,
        dockerfile,
        qloudinst,
        deploy,
        project,
        applications,
        components,
        box,
        dump,
        ticket,
        get_ticket_from_stand_name,
        get_ticket_from_last_commit_message,
        get_ticket_from_current_branch_name,
        domain_tpl,
        domain_type,
        deploy_hook,
        pull_base_image,
        var,
):
    # Тэг образа и название окружения совпадают с именем стенда.
    version = environment = standname
    if deploy or box:
        version = f'{version}_{uuid.uuid4()}'

    ticket = get_stand_ticket(
        standname=standname,
        ticket=ticket,
        get_ticket_from_stand_name=get_ticket_from_stand_name,
        get_ticket_from_last_commit_message=get_ticket_from_last_commit_message,
        get_ticket_from_current_branch_name=get_ticket_from_current_branch_name,
    )

    deploy_comment = ticket or utils.NO_DEPLOY_COMMENT

    steps = [
        Step(
            'build',
            image=image,
            buildfile=buildfile,
            sandbox=sandbox,
            dockerfile=dockerfile,
            version=version,
            version_build_arg=version_build_arg,
            pull_base_image=pull_base_image,
        ),
        Step('push', sandbox=sandbox, image=image, version=version),
        Step(
            'deploy',
            image=image,
            version=version,
            from_version=None,
            qloudinst=qloudinst,
            deploy=deploy,
            project=project,
            applications=applications,
            environment=environment,
            components=components,
            box=box,
            deploy_comment_format=deploy_comment,
            target_state=None,
            deploy_draft=False,
            dump=dump,
            pull_vcs=False,
            var=var or None,
        ),
    ]
    if domain_tpl is not None:
        for application in applications:
            steps.append(
                Step(
                    'add-domain',
                    qloudinst=qloudinst,
                    deploy=deploy,
                    project=project,
                    application=application,
                    environment=environment,
                    domain=domain_tpl.format(
                        standname=standname,
                        application=application,
                    ),
                    domain_type=domain_type,
                )
            )
    if deploy_hook is not None:
        for application in applications:
            steps.append(
                Step(
                    'add-deploy-hook',
                    qloudinst=qloudinst,
                    project=project,
                    application=application,
                    environment=environment,
                    deploy_hook=deploy_hook,
                )
            )
    run_workflow(
        cmd='stand',
        steps=steps,
        skip=skip,
        dry_run=dry_run,
        no_rollback=no_rollback,
    )


def get_stand_ticket(
        standname,
        ticket,
        get_ticket_from_stand_name,
        get_ticket_from_last_commit_message,
        get_ticket_from_current_branch_name,
):
    if ticket:
        ticket_key = utils.parse_ticket_key(ticket)
        if ticket_key:
            return ticket_key

    if get_ticket_from_stand_name:
        ticket_key = utils.parse_ticket_key(standname)
        if ticket_key:
            return ticket_key

    def get_ticket_from_git_command_output(*args, **kwargs):
        try:
            result = sh.git(*args, **kwargs)
        except sh.ErrorReturnCode_128:
            return None
        stdout = result.stdout.decode('utf-8')
        return utils.parse_ticket_key(stdout)

    if get_ticket_from_last_commit_message:
        ticket_key = get_ticket_from_git_command_output('log', '-1', '--pretty=%B', _tty_out=False)
        if ticket_key:
            return ticket_key

    if get_ticket_from_current_branch_name:
        ticket_key = get_ticket_from_git_command_output('rev-parse', '--abbrev-ref', 'HEAD', _tty_out=False)
        if ticket_key:
            return ticket_key

    return None


@click.command(
    help='return stand ticket'
)
@options.stand_option
@options.get_ticket_from_stand_name
@options.get_ticket_from_last_commit_message
@options.get_ticket_from_current_branch_name
def stand_ticket(
        standname,
        get_ticket_from_stand_name,
        get_ticket_from_last_commit_message,
        get_ticket_from_current_branch_name,
):
    ticket = get_stand_ticket(
        standname=standname,
        ticket=None,
        get_ticket_from_stand_name=get_ticket_from_stand_name,
        get_ticket_from_last_commit_message=get_ticket_from_last_commit_message,
        get_ticket_from_current_branch_name=get_ticket_from_current_branch_name,
    )
    click.echo(ticket)

# coding: utf-8
"""
Module to define all used `click.option`s
"""

import click

from tools.releaser.src.conf import cfg
from tools.releaser.src.cli import utils


git_remote_option = click.option(
    '--remote', '-r',
    help='git remote name',
    default=cfg.REMOTE,
    show_default=True,
)
direct_push_option = click.option(
    '--direct-push',
    is_flag=True,
    help='For arc, push directly to the current remote (not likely to work for `trunk`). Useful for release branches.',
    default=None,
    show_default=True,
)
version_option = click.option(
    '--version', '-v',
    help='explicitly specified new version name',
    default=None,
)
from_version_option = click.option(
    '--from-version',
    help='explicitly specified previous version (e.g. for changelog)',
    default=None,
)
version_schema = click.option(
    '--version-schema',
    help='describe versioning schema "major.minor" by default ',
    default=None,
)
release_type = click.option(
    '--release-type',
    help='which part of version to increase',
    default=None,
)
version_build_arg_option = click.option(
    '--version-build-arg/--no-version-build-arg',
    help='add "--build-arg APP_VERSION=<version>" in docker build',
    is_flag=True,
    default=cfg.VERSION_BUILD_ARG,
    show_default=True,
)
non_interactive_option = click.option(
    '--non-interactive',
    help='continue without confirmation',
    is_flag=True,
    default=cfg.NON_INTERACTIVE,
    show_default=True,
)
image_option = click.option(
    '--image', '-i',
    help='image name, suffix of the image url %s/<project>/<image_name> or full image url' % cfg.REGISTRY_HOST,
    type=click.types.FuncParamType(lambda x: utils.get_image_url(x)),
    default=cfg.IMAGE,
    show_default=True,
)
pull_base_image_option = click.option(
    '--pull-base-image',
    help='pull the latest version of a base image on build',
    is_flag=True,
    default=cfg.PULL_BASE_IMAGE,
    show_default=True,
)
pull_vcs_option = click.option(
    '--pull-vcs',
    help='pull changes from remote',
    is_flag=True,
    default=cfg.PULL_VCS,
    show_default=True,
)
buildfile_option = click.option(
    '--buildfile',
    help='build file name for ya package',
    type=click.Path(dir_okay=False, readable=True, allow_dash=False, resolve_path=True),
    default=cfg.BUILDFILE,
    show_default=True,
)
sandbox_option = click.option(
    '--sandbox',
    help='build in sandbox',
    is_flag=True,
    default=bool(cfg.SANDBOX_CONFIG) or bool(cfg.SANDBOX),
    show_default=True,
)
dockerfile_option = click.option(
    '--dockerfile', '-f',
    help='Dockerfile name',
    type=click.Path(dir_okay=False, readable=True, allow_dash=False),
    default=cfg.DOCKERFILE,
    show_default=True,
)
qloud_instance_option = click.option(
    '--qloudinst', '-q',
    help='Qloud instance name',
    default=cfg.QLOUDINST,
    type=click.Choice([cfg.QloudInstance.INT, cfg.QloudInstance.EXT]),
    show_default=True,
)
project_option = click.option(
    '--project', '-p',
    help='Qloud project name',
    default=cfg.PROJECT,
    show_default=True,
)
application_option = click.option(
    '--application', '-a',
    help='Qloud application or Y.Deploy project',
    default=cfg.APPLICATION,
    required=True,
    show_default=True,
)
applications_option = click.option(
    '--applications', '-a',
    help='comma-separated list of qloud applications or Y.Deploy projects',
    default=cfg.APPLICATIONS,
    callback=lambda ctx, param, value: value.strip().split(','),
    required=True,
    show_default=True,
)
environment_option = click.option(
    '--environment', '--stage', '-e',
    help='Qloud environment or Y.Deploy stage',
    default=cfg.STAGE or cfg.ENVIRONMENT,
    required=True,
    show_default=True
)
environment_target_state_option = click.option(
    '--target-state', '-t',
    help='target state',
    default=cfg.TARGET_STATE,
    type=click.types.FuncParamType(lambda x: x.upper()),
    required=False,
    show_default=True
)
env_vars_option = click.option(
    '--var',
    help='environment variable (--var DB_HOST=some-db.in.pgaass)',
    multiple=True,
)
components_option = click.option(
    '--components', '--deploy-units', '-c',
    help='comma-separated list of Qloud components or Y.Deploy deploy units',
    default=cfg.DEPLOY_UNITS or cfg.COMPONENTS,
    callback=lambda ctx, param, value: utils.parse_components(value),
    show_default=True,
    required=True,
)
domain_option = click.option(
    '--domain', '-d',
    help='domain name',
    default=None,
    show_default=False,
    required=True,
)
deploy_hook_option = click.option(
    '--deploy-hook',
    help='deploy hook url (i.e. https://qooker.yandex-team.ru/hook)',
    default=cfg.DEPLOY_HOOK,
    show_default=True,
    required=False,
)
domain_tpl_option = click.option(
    '--domain-tpl',
    help='domain template name',
    default=cfg.DOMAIN_TPL,
    show_default=False,
    required=False,
)
domain_type_option = click.option(
    '--domain-type',
    help='qloud balancer or awacs namespace (like `tools-int-test`)',
    default=cfg.DOMAIN_TYPE,
    show_default=False,
    required=False,
)
environment_dump_from_option = click.option(
    '--dump',
    help='path to file with environment dump',
    type=click.Path(dir_okay=False, readable=True, allow_dash=False),
    default=cfg.ENVIRONMENT_DUMP_FROM,
    show_default=False,
    required=False,
)
environment_dump_to_option = click.option(
    '--dump',
    help='path to file with environment dump',
    type=click.Path(dir_okay=False, readable=True, allow_dash=False),
    default=None,
    show_default=False,
    required=True,
)
deploy_comment_format_option = click.option(
    '--deploy-comment-format', '-dcf',
    help='deploy comment format',
    default=cfg.DEPLOY_COMMENT_FORMAT,
    show_default=True,
    required=False,
)
stand_option = click.option(
    '--standname', '-s',
    help='Qloud environment or Y.Deploy stage',
    default=cfg.STANDNAME,
    show_default=True,
    required=True
)
trace_option = click.option(
    '--trace', '-t',
    is_flag=True,
    help='trace command',
    default=False,
    show_default=True,
)
dry_run_option = click.option(
    '--dry-run', '-N',
    is_flag=True,
    help='pretend to run command, but only print stuff',
    default=None,
    show_default=True,
)
deploy_draft_option = click.option(
    '--deploy-draft', '-dd',
    is_flag=True,
    help='create draft in deploy instead on automatically apply it',
    default=cfg.DEPLOY_DRAFT,
    show_default=True,
)
workflow_skip_option = click.option(
    '--skip',
    help='comma-separated list of workflow steps to skip',
    default=None,
    required=False,
)
workflow_no_rollback_option = click.option(
    '--no-rollback',
    help='Disable automatically rollback workflow step on exceptions',
    is_flag=True,
    default=None,
    required=False,
)
version_file_path = click.option(
    '--version-file-path',
    help='path to file',
    type=click.Path(file_okay=True, writable=True, exists=True),
    default=cfg.VERSION_FILES.keys(),
    show_default=True,
    required=False,
    multiple=True,
)
ticket = click.option(
    '--ticket',
    help='Tracker ticket key',
    default=None,
)
get_ticket_from_stand_name = click.option(
    '--get-ticket-from-stand-name',
    help='Try to get Tracker ticket key from stand name',
    is_flag=True,
    default=False,
    show_default=True,
)
get_ticket_from_last_commit_message = click.option(
    '--get-ticket-from-last-commit-message/--no-get-ticket-from-last-commit-message',
    help='Try to get Tracker ticket key from last commit message',
    is_flag=True,
    default=True,
    show_default=True,
)
get_ticket_from_current_branch_name = click.option(
    '--get-ticket-from-current-branch-name',
    help='Try to get Tracker ticket key from current branch name',
    is_flag=True,
    default=False,
    show_default=True,
)
ssh_command = click.option(
    '--command',
    help='Run command',
    default='',
)
pssh_cmd = click.option(
    '--pssh-cmd',
    help='actual command to run with the hostlist (e.g. `pscp`, `pslurp`, `prsync`)',
    default='pssh',
    show_default=True,
)
cmd_shell_wrap_command = click.option(
    '--shellwrap', '-s',
    help='Automatically wrap the supplied subcommand in a shell',
    is_flag=True,
    default=False,
    show_default=True,
)
box_option = click.option(
    '-b',
    '--box',
    help='Y.Deploy box name',
    default=cfg.BOX,
    show_default=True,
    required=False,
)
deploy_option = click.option(
    '--deploy',
    help='Use Y.Deploy instead of Qloud (better use `deploy: true` in config)',
    is_flag=True,
    default=bool(cfg.DEPLOY),
    show_default=True,
)
tvm_client_id_option = click.option(
    '--tvm-client-id',
    help='tvm client id',
    default=cfg.TVM_CLIENT_ID,
    show_default=True,
)
logroker_topic_option = click.option(
    '--logbroker-topic',
    help='logbroker topic',
    default=cfg.LOGBROKER_TOPIC,
    show_default=True,
)

import dataclasses
import inspect
import logging
import pkgutil
import time
from importlib import import_module
from pprint import pformat
from types import ModuleType
from typing import List

import click
import reactor_client
import vh

from cloud.dwh.nirvana import reactor
from cloud.dwh.nirvana.config import BaseDeployConfig
from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.config import NirvanaRunConfig
from cloud.dwh.utils.log import setup_logging

LOG = logging.getLogger(__name__)

DEFAULT_VH_MODULE = 'cloud.dwh.nirvana.vh'
ALL_MODULES = '*'


class DeployMode:
    SINGLE_MODULE = 'single_module'
    ALL_MODULES = 'all_modules'


def setup_reaction(config: BaseDeployConfig, ctx: DeployContext):
    if not ctx.run_config.workflow_guid and not click.confirm('Workflow guid is not specified in "parameters.yaml". Do you want to create new workflow?'):
        LOG.warning('Canceled!')
        exit(1)

    reactor_wrapper = reactor.ReactorWrapper(token=config.nirvana_oauth_token)

    nirvana_base_config = NirvanaRunConfig(
        workflow_ns_id=config.nirvana_workflow_group_ns_id,
        oauth_token=config.nirvana_oauth_token,
        threaded_api=config.use_threaded_api,
        workflow_tags=config.nirvana_tags + tuple(ctx.run_config.workflow_tags or ()),
        quota=config.nirvana_quota,
    )

    run_config = {**dataclasses.asdict(ctx.run_config), **dataclasses.asdict(nirvana_base_config)}

    builder = reactor_client.reaction_builders.NirvanaReactionBuilder()
    builder.set_version(2)

    if ctx.graph:
        # add / update workflow in Nirvana
        run_config['graph'] = ctx.graph
        keeper = vh.run(**run_config)
        info = keeper.get_workflow_info()
        builder.set_source_graph(instance_id=info.workflow_instance_id)
        LOG.info('Nirvana workflow guid: %s', info.workflow_id)
        LOG.info('Deployed nirvana workflow "%s"', f'[vh] {ctx.run_config.label}')
    else:
        builder.set_source_graph(instance_id=run_config['workflow_guid'])

    if not ctx.reaction_path:
        LOG.warning('No reaction specified!')
        return

    builder.set_owner(config.reactor_owner)
    builder.set_reaction_path(ctx.reaction_path)

    builder.set_instance_ttl(ctx.nirvana_instances_ttl)

    if ctx.cleanup_ttl:
        builder.set_cleanup_strategy(
            reactor_client.r_objs.CleanupStrategyDescriptor(
                cleanup_strategies=[
                    reactor_client.r_objs.CleanupStrategy(
                        ttl_cleanup_strategy=reactor_client.r_objs.TtlCleanupStrategy(ttl_days=ctx.cleanup_ttl),
                    ),
                ],
            ),
        )

    builder.set_expression_after_success(ctx.success_expression)
    builder.set_expression_after_failure(ctx.failure_expression)

    if ctx.retries:
        builder.set_retry_policy(
            retries=ctx.retries,
            time_param=ctx.retry_time_param,
            retry_policy_descriptor=ctx.retry_policy,
        )

    if ctx.schedule:
        builder.trigger_by_cron(cron_expr=ctx.schedule, misfire_policy=ctx.schedule_misfire_policy)

    if ctx.input_triggers:
        builder.trigger_by_inputs(**{ctx.input_triggers_type.value: True})

        for input_trigger in ctx.input_triggers:
            builder.set_block_param_to_artifact(
                operation_id=input_trigger.block_code,
                param_name=input_trigger.param_name,
                artifact_reference=reactor_client.r_objs.ArtifactReference(
                    namespace_identifier=reactor_client.r_objs.NamespaceIdentifier(namespace_path=input_trigger.artifact.path),
                )
            )

    if ctx.artifact:
        reactor_wrapper.create_artifact(ctx.artifact)

    reactor_wrapper.replace_reaction(builder)
    reactor_wrapper.add_notifications(builder, config.environment)

    if ctx.sequential_queue:
        sequential_queue = reactor_wrapper.create_queue(reactor.Queue(path=ctx.sequential_queue))
        reactor_wrapper.add_to_queue(builder, sequential_queue)


def deploy_module(module: ModuleType, config: BaseDeployConfig):
    LOG.info('====================================================')
    LOG.info('Deploying module: %s', module)

    LOG.debug('Build nirvana graph...')
    ctx: DeployContext = module.main(config)
    LOG.debug('Nirvana graph built!')

    LOG.debug('Deploying reaction...')
    setup_reaction(config, ctx)
    LOG.debug('Reaction deployed!')

    LOG.info(f'Done deploying: {module}')


def deploy_single_module(config: BaseDeployConfig):
    imported_module = import_module(f'{config.vh_workflows_module}.{config.module}')

    LOG.warning('Will be deployed module %s to %s environment', imported_module.__name__, config.environment.upper())

    if not click.confirm('Continue?'):
        LOG.warning('Canceled!')
        exit(1)

    deploy_module(imported_module, config)


def find_workflow_modules(root: str) -> List[ModuleType]:
    wf_modules = set()

    def loop(pkg_name):
        pkg = import_module(pkg_name)
        pkg_list = [name for (_, name, is_pkg) in pkgutil.iter_modules(pkg.__path__) if is_pkg]
        if len(pkg_list) == 0:
            if hasattr(pkg, 'main') and inspect.isfunction(getattr(pkg, 'main')):
                wf_modules.add(pkg)
        else:
            for name in pkg_list:
                loop(pkg_name + '.' + name)

    loop(root)

    return sorted(wf_modules, key=lambda m: m.__name__)


def deploy_all_modules(root: str, config: BaseDeployConfig):
    wf_modules = find_workflow_modules(root)
    LOG.warning('Will be deployed these modules to %s environment:\n%s', config.environment.upper(), '\n'.join(map(str, wf_modules)))

    if not click.confirm('Continue?'):
        LOG.warning('Canceled!')
        exit(1)

    for module in wf_modules:
        deploy_module(module, config)
        time.sleep(5)


def get_deploy_config(vh_module: str,
                      environment: str,
                      nirvana_oauth_token: str,
                      use_threaded_api: bool,
                      module: str,
                      deploy_mode: str,
                      debug: bool,
                      **override_config) -> BaseDeployConfig:
    config_module_path = f'{vh_module}.config.{environment}'

    LOG.info("Trying to import %s.DeployConfig config module", config_module_path)

    imported_config_module = import_module(config_module_path)
    if not hasattr(imported_config_module, 'DeployConfig'):
        raise ImportError(f'In module {config_module_path} class DeployConfig not found')

    deploy_config = imported_config_module.DeployConfig(
        vh_module=vh_module,
        environment=environment,
        nirvana_oauth_token=nirvana_oauth_token,
        use_threaded_api=use_threaded_api,
        module=module,
        deploy_mode=deploy_mode,
        debug=debug,
        **dict(override_config),
    )

    return deploy_config


@click.command()
@click.option('--vh-module', help='Module with vh workflows and configs. e.g "cloud.dwh.nirvana.vh"',
              default=DEFAULT_VH_MODULE)
@click.option('--env', type=str, help='Environment. Must be in vh/config folder', required=True)
@click.option('--nirvana-token', help='Nirvana oauth token. Default from $NIRVANA_TOKEN',
              envvar='NIRVANA_TOKEN', required=True)
@click.option('--override-config', help='Config overrides. E.g. --override-config yt_token makhalin_yt-token',
              type=(str, str), multiple=True)
@click.option('--use-threaded-api', default=True, help='Use threaded_api option in vh.run', is_flag=True)
@click.option('--module', help='Module to deploy without prefix "{vh-module}.workflows"', required=True)
@click.option('--debug', help='Print more logs', is_flag=True)
def main(vh_module: str, env: str, nirvana_token: str, override_config, use_threaded_api: bool, module: str, debug: bool):
    setup_logging(debug=debug, colored=True)

    config = get_deploy_config(
        vh_module=vh_module,
        environment=env,
        nirvana_oauth_token=nirvana_token,
        use_threaded_api=use_threaded_api,
        module=module,
        deploy_mode=DeployMode.ALL_MODULES if module.endswith(ALL_MODULES) else DeployMode.SINGLE_MODULE,
        debug=debug,
        **dict(override_config),
    )

    LOG.info('Deploy config:\n%s', pformat(dataclasses.asdict(config), indent=1, width=1))

    if config.deploy_mode == DeployMode.SINGLE_MODULE:
        LOG.info('Deploying module %s', config.module)
        deploy_single_module(config)
    elif config.deploy_mode == DeployMode.ALL_MODULES:
        root = f'{config.vh_workflows_module}.{config.module.rsplit(ALL_MODULES, maxsplit=1)[0]}'.strip('.')
        LOG.info('Deploying all modules from %s', root)
        deploy_all_modules(root, config)

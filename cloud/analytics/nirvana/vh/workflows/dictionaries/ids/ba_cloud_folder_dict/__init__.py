import json

import vh
from cloud.dwh.nirvana import reactor
from cloud.dwh.nirvana.config import DeployContext
from library.python import resource

from cloud.analytics.nirvana.vh import operations
from cloud.analytics.nirvana.vh.config.base import BaseDeployConfig as DeployConfig

WORKFLOW_NAME = 'Dictionaries. BA-cloud-folder'
REACTION_NAME = 'ba_cloud_folder_dict'
REACTION_SUBDIR = 'dictionaries/yt/'
REACTION_CRON = '0 0 * * * ? *'
REACTION_CRON_MISFIRE_POLICY = reactor.reactor.r_objs.MisfirePolicy.FIRE_ONE


def fill_graph(graph: vh.Graph, config: DeployConfig):
    parameters = json.loads(resource.find('dictionaries/ids/ba_cloud_folder_dict/resources/parameters.json'))
    parameters = parameters[config.environment]

    output_mr_table_op = vh.op(id=operations.GET_MR_TABLE)
    output_mr_table_result = output_mr_table_op(
        _options={
            'cluster': config.yt_cluster,
            'yt-token': config.yt_token,
            'table': parameters['destination_path'],
        },
    )

    datetime_sql_file = vh.data_from_str(content=resource.find('yql/utils/datetime.sql'), name='datetime.sql')
    archive_op = operations.create_tar_archive(
        files=[('datetime.sql', datetime_sql_file)],
    )

    yql_op = vh.op(id=operations.YQL_1)
    yql_op(
        _inputs={
            'input1': output_mr_table_result['outTable'],
            'files': [archive_op['archive']],
        },
        _options={
            'request': resource.find('dictionaries/ids/ba_cloud_folder_dict/resources/query.sql'),
            'mr-default-cluster': config.yt_cluster,
            'mr-account': config.mr_account,
            'yt-token': config.yt_token,
            'yql-token': config.yql_token,
            'param': [f'{name}={param}' for name, param in parameters.items()],
            'mr-output-path': config.yt_tmp_path,
            'yt-owners': config.yt_owners,
            'yt-pool': config.yt_pool,
            'timestamp': vh.OptionExpr('${datetime.timestamp}'),
        },
    )

    run_config = dict(
        label=WORKFLOW_NAME,
        workflow_tags=('dictionaries',),
    )

    reaction_prefix = f'{config.reactor_path_prefix}{REACTION_SUBDIR}'
    reaction_path = f'{reaction_prefix}{REACTION_NAME}'

    artifact_name = f'{REACTION_NAME}_ready'
    artifact_path = f'{reaction_prefix}{artifact_name}'
    artifact = reactor.Artifact(path=artifact_path)
    artifact_result = parameters['destination_path']

    return DeployContext(
        run_config=run_config,
        graph=graph,
        reaction_path=reaction_path,
        retries=10,
        schedule=REACTION_CRON,
        schedule_misfire_policy=REACTION_CRON_MISFIRE_POLICY,
        artifact=artifact,
        artifact_success_result=artifact_result,
    )


def main(config: DeployConfig):
    with vh.Graph() as graph:
        return fill_graph(graph=graph, config=config)

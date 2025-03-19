from typing import Callable, cast, List, Dict, Any, Optional, TypeVar
from functools import wraps, partial
import logging

import vh
from vh.frontend.nirvana import OpPartial

logger = logging.getLogger(__name__)


Function = TypeVar('Function', bound=Callable[..., Any])


# config for operations
write_conf = dict(
    yt_token='robot-clanalytics-yt',
    yql_token='robot-clanalytics-yql',
    solomon_token='robot-clanalytics-solomon',
    mr_default_cluster='hahn',
    mr_account='cloud_analytics',
    yt_pool='cloud_analytics_pool'
)


# spyt clusters params
spyt_dwh_discovery_path = '//home/cloud_analytics/spark-discovery/spark3'
spyt_adhoc_discovery_path = '//home/cloud_analytics/spark-discovery/adhoc'


# operation for extracting files from uploaded tar-arhive
extract_op = vh.op(id='cb958bd8-4222-421b-8e83-06c01897a725')


# operation for packing files to zip-arhive
zip_op = vh.op(id='de667809-b321-4aeb-b11c-d454aaf83653')


# operation for saving file to yt path
yt_write_file = partial(
    vh.op(id='c5c7e0e2-ceb4-4dd8-b240-cca0d2a9e183'),
    mr_account=write_conf['mr_account'],
    yt_pool=write_conf['yt_pool'],
    mr_default_cluster=write_conf['mr_default_cluster'],
    yt_token=write_conf['yt_token']
)


# actual spyt-submit operation id
spark_spark3_op_id = 'f11f4ac0-5f8c-4b87-aa32-a628442b1ddb'
spark_adhoc_op_id = '860d235f-ac3c-460f-9fc5-1d0a214c9b1b'


# spyt operation with given params
def spark_op(**kwargs):  # type: ignore
    """Generates Spyt Operaiton in Nirvana:

    Some arguments:
        cluster: must be one of `spark3` or `adhoc`. Defaults to `spark3`. Affects on running in cluster mode
    """
    spark_conf = dict(retries_on_job_failure=3,
                      spyt_deploy_mode="cluster",
                      spyt_yt_token=write_conf['yt_token'],
                      spyt_proxy=write_conf['mr_default_cluster'],
                      spyt_secret=write_conf['yt_token'],
                      spyt_spark_conf=[
                          'spark.pyspark.python=/opt/python3.7/bin/python3.7',
                          'spark.jars=yt:///home/sashbel/graphframes-assembly-0.8.2-SNAPSHOT-spark3.0.jar'
                      ])

    if 'spyt_deps_dir' in kwargs:
        kwargs['spyt_py_files'] = f'yt:/{kwargs["spyt_deps_dir"]}/dependencies.zip'
        if 'spyt_driver_filename' in kwargs:
            kwargs['spyt_driver_file'] = f'yt:/{kwargs["spyt_deps_dir"]}/{kwargs["spyt_driver_filename"]}'
            kwargs.pop('spyt_driver_filename')
        kwargs.pop('spyt_deps_dir')

    if 'spyt_yt_token' in kwargs:
        spark_conf.pop('spyt_yt_token')

    if 'spyt_secret' in kwargs:
        spark_conf.pop('spyt_secret')

    if ('cluster' in kwargs) and (kwargs['cluster'] == 'adhoc'):
        kwargs['spyt_discovery_path'] = spyt_adhoc_discovery_path
        kwargs.pop('cluster')
        spark_op_id = spark_adhoc_op_id
    else:
        kwargs['spyt_discovery_path'] = spyt_dwh_discovery_path
        spark_op_id = spark_spark3_op_id
    conf = {**spark_conf, **kwargs}
    return vh.op(id=spark_op_id)(**conf)


pulsar_add_instance_op = vh.op(id="087645e8-8de6-4f19-be2b-5d943faff859")
extract_metrics = vh.op(id='e9e61c88-7fab-47dd-9519-7abf5316ac8f')


# actual container id (py37analytics)
nirvana_containers = {
    'base': '50c65aff-a430-48ce-a4d8-7ce676ca18d1',
    'base-0.1': '9454aad6-37ca-4ce7-a269-5aa96d104719'
}


# actual porto-layer for joblayer operations
container_porto = vh.Porto([vh.data_from_str(nirvana_containers['base'], data_type='binary')])


# bash-script for joblayer operation (extracting tar-arhive and running given python-script with some args)
def extract_and_run(script: str, script_args: Optional[List[str]] = None) -> str:
    args = '' if (script_args is None) else ' '.join(script_args)
    return f'tar xvf input &&  PYTHONPATH=./src:${{PYTHONPATH}} && python3.7 {script} {args}'


# decorator for vh.Op object with definition of some arguments
def preprocess_args(func: Function) -> Function:
    @wraps(func)
    def wrapper(*args, **kwargs):  # type: ignore
        if 'script_args' in kwargs:
            script_args = kwargs.pop('script_args')
        else:
            script_args = None
        run_script = extract_and_run(kwargs['script'], script_args)
        kwargs['script'] = run_script
        if 'job_layer' in kwargs:
            ver = kwargs['job_layer']
            if ver not in nirvana_containers:
                raise ValueError(f'Conatainre version "{ver}" is unknown.\nUse on of {nirvana_containers.keys()}')
            kwargs['job_layer'] = [nirvana_containers[ver]]
        else:
            kwargs['job_layer'] = [nirvana_containers['base']]
        result = func(*args, **kwargs)
        return result
    return cast(Function, wrapper)


# run_job operation with defined arguments
run_job_op = preprocess_args(vh.op(id='1e086883-0167-451d-bc51-949bf1192e07'))


# small (given less resources) run_job operation with defined arguments
run_small_job_op = preprocess_args(vh.op(id='49fb3c0c-0b64-415e-bfaa-f3bea57c056d'))

current_date_op = vh.op(id='90f0163b-2523-4a42-9b3c-6d9a5797d038')


tm_conf = dict(mdb_cluster_id='07bc5e8c-c4a7-4c26-b668-5a1503d858b9',
               mdb_oauth_token='robot_clanalytic-mdb',
               ch_user='robot_clanalytic',
               ch_password='robot-clanalytics-clickhouse',
               yt_token='robot-clanalytics-yt',
               yt_pool='cloud_analytics_pool',
               yt_tmp_dir='//home/cloud_analytics/tmp/transfer_manager',
               reset_state=True,
               retries_on_job_failure=2,
               default_retries_count=2,
               use_tm=True,
               ch_rewrite_all_table=True)

tm_yt2ch_copy = vh.op(id='7449357d-91dc-426d-89dd-46722ce500b2')


def run_yql_script(verbose: bool = False, **kwargs: Dict[str, Any]) -> OpPartial:
    """Run yql operation

    :param query: query to run
    :param query: path to query to run (overwrites param `query`)
    :param encryptedOauthToken: yql-token
    :param yql_token: also yql-token (overwrites param `encryptedOauthToken`)
    :param verbose: flag to display yql-query, defaults to False
    :return: operation
    """
    if 'yql_token' in kwargs:
        kwargs['encryptedOauthToken'] = kwargs.pop('yql_token')
    if 'path' in kwargs:
        path = kwargs.pop('path')
        with open(path, 'r') as fin:  # type: ignore
            kwargs['query'] = fin.read()
    if verbose:
        logger.debug(kwargs['query'])
    return vh.op(id='df2f740c-c530-441c-b87a-b6890be8d71c')(**kwargs)


# Python Deep Learning allows to execute any python script, contains preinstalled deep learning libraries
python_dl_base = partial(vh.op(id='317154dd-e4b0-4ce1-9c20-69f09bd0df78'), yt_token='robot-clanalytics-yt')

# get MR table path operation
get_mr_table = partial(vh.op(id='6ef6b6f1-30c4-4115-b98c-1ca323b50ac0'), cluster=write_conf['mr_default_cluster'], yt_token=write_conf['yt_token'])


# get MR directory operation
get_mr_dir = partial(vh.op(id='eec37746-6363-42c6-9aa9-2bfebedeca60'), cluster=write_conf['mr_default_cluster'], yt_token=write_conf['yt_token'])


# transfering data from Solomon to YT operation
solomon_to_yt = partial(vh.op(id='178c5665-d252-4ca6-ad7d-be8f8e9e30d3'), yt_token=write_conf['yt_token'],
                        yt_pool=write_conf['yt_pool'], solomon_token=write_conf['solomon_token'])


# operation for downloading .bin-file from yt
get_bin_from_yt= partial(vh.op(id='8a96b580-027e-4983-8b73-5c1cc979f94b'),
                         mr_default_cluster=write_conf['mr_default_cluster'], yt_token=write_conf['yt_token'], yt_pool=write_conf['yt_pool'])


# CatBoost: Train operation with given arguments
catb_train = partial(vh.op(id='51e53d18-5e4b-454b-849f-02ed4eede7be'), yt_token=write_conf['yt_token'], yt_pool=write_conf['yt_pool'], cpu_guarantee=1600)


# CatBoost: Apply operation with given arguments
catb_apply = partial(vh.op(id='72f5410e-481e-4825-9d87-4df32babcfbf'), yt_token=write_conf['yt_token'], yt_pool=write_conf['yt_pool'], cpu_guarantee=100)


def copy_to_ch(yt_table, ch_table, ch_table_schema, ch_primary_key, ch_partition_key=None, yt_table_is_file=False):  # type: ignore
    mr_table = get_mr_table(table=yt_table) if not yt_table_is_file else get_mr_table(fileWithTableName=yt_table)
    return tm_yt2ch_copy(yt_table=mr_table,
                         ch_table=f"cloud_analytics.{ch_table}",
                         ch_primary_key=ch_primary_key,
                         ch_partition_key=ch_partition_key,
                         ch_schema=ch_table_schema,
                         **tm_conf)

import os
from typing import List
from typing import Optional
from typing import Tuple

import vh

from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.config.base import PG_DATABASE
from cloud.dwh.nirvana.vh.config.base import PG_HOST
from cloud.dwh.nirvana.vh.config.base import PG_PASSWORD_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import PG_PORT
from cloud.dwh.nirvana.vh.config.base import PG_USER
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_CONFIG
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_PY_FILES
from cloud.dwh.nirvana.vh.config.base import SPYT_DISCOVERY_DIR
from cloud.dwh.nirvana.vh.config.base import SPYT_JARS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SPYT_YT_PROXY
from cloud.dwh.nirvana.vh.config.base import YAV_OAUTH_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import YT_OAUTH_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.utils import datetimeutils

# /Library/utl
BOOL_PARAM_TO_OPTIONAL_OUTPUT = "2c07ed65-a2a7-4771-a184-b59ada41ca73"

# /Library/util/archive
CREATE_TAR_10 = '961eda13-7e75-4e8e-96aa-fa43c1dd97c1'  # Create TAR archive 10
EXTRACT_FROM_TAR = 'cb958bd8-4222-421b-8e83-06c01897a725'  # Extract from tar

# /Library/util/branching
# if (condition) then 1st branch else 2nd branch
IF_BRANCHING_CONDITION = '9c02845d-1c79-4bba-a74d-a8a06296a48d'
# if condition: MR Path exists
IF_BRANCHING_MR_PATH_EXISTS = '848a554b-3ebe-471b-aaa6-cd6e1d9c04bf'

# /Library/util/datetime
GET_TIMESTAMP_NOW = '58d8d7a2-a7a2-44da-8402-a503472d593e'  # Get Timestamp Now

# /Library/util/json
# Convert Text To Json Option
CONVERT_TEXT_TO_JSON_OPTION = 'b3cf53c0-bdf8-466f-a695-8c582183e0fb'
# Extract Json Field as Text
EXTRACT_JSON_FIELD_AS_TEXT = '35c33221-be36-4b0b-81ba-84614355dc38"'
# JSON merge objects into one
JSON_MERGE_OBJECTS_INTO_ONE = '1f8f43f5-73ee-4868-92a6-cdee13e7b3dd'
JSON_TO_YQL_PARAM = '16083719-3f10-4c9a-9ab0-fce70a6dde2c'  # Json to yql param
# Json Unwrap Single Object
JSON_UNWRAP_SINGLE_OBJECT = 'dfd39fac-ef76-4b25-b1fe-53ea30e9b8f6'
# [Light] Groovy Json Process (2 inputs) (deterministic)
LIGHT_GROOVY_JSON_PROCESS_2_INPUTS_DETERMINISTIC = '61543d9d-42a0-11e7-89a6-0025909427cc'
# Single Option To Json Output
SINGLE_OPTION_TO_JSON_OUTPUT = '2fdd4bb4-4303-11e7-89a6-0025909427cc'
# python script to TSV, JSON, TEXT
PYTHON_SCRIPT_TO_TSV_JSON_TEXT = '47129f6c-aba2-4fae-88ea-eadeb457d4f2'

# /Library/util/python
PYTHON_EXECUTOR = "328f78e9-059d-11e7-a873-0025909427cc"  # Python executor
SECRET_TO_FILE = "cebca7a8-ac9b-418e-ba54-10f34b62c985"  # Secret to file

# /Library/util/text
# Single Option To Text Output
SINGLE_OPTION_TO_TEXT_OUTPUT = '2417849a-4303-11e7-89a6-0025909427cc'

# /Library/util/typecast
ANY_TO_MR_TABLE = '3326d2c0-f5c9-47b5-aa26-e3216b6f3775'  # Convert any to MR Table

# /Library/util/vcs
# SVN: Checkout (Deterministic)
SVN_CHECKOUT_DETERMINISTIC = '15c7eab1-aaf1-49ad-b196-beaa9a8cf3ab'

# /Library/services/solomon
PUSH_JSON_TO_SOLOMON = '77806b43-0dce-448d-8b27-9f5c120487d2'

# /Library/services/yql
YQL_1 = 'b189dddb-c3bd-4bdb-bc29-4cc509c6a9e3'  # YQL 1
YQL_2 = '4494caaf-1915-4e8b-ad9f-f4a1fbe2c1ae'  # YQL 2
YQL_WITH_JSON_OUTPUT = '2b59733b-7fb2-4ad1-94e3-69578fbf9242'  # YQL WITH JSON OUTPUT

# /Library/services/yt
GET_MR_DIRECTORY = 'eec37746-6363-42c6-9aa9-2bfebedeca60'  # Get MR Directory

# /Library/services/yt/operations
MR_MERGE = 'c1702028-57a7-4ad0-95fd-e2c76de541fd'  # MR Merge

# /Library/services/yt/tables
GET_MR_TABLE = '6ef6b6f1-30c4-4115-b98c-1ca323b50ac0'  # Get MR Table
MR_COPY_TABLE = '2012c51e-f743-4130-bbf2-00c9ef146560'  # MR Copy Table
MR_DROP = '2a753dbe-40da-4db8-880c-57321f474151'  # MR Drop

# Custom
PYSPYT = 'f11f4ac0-5f8c-4b87-aa32-a628442b1ddb'  # SPYT
PYSPYT_1SECRET = '279d33c9-abb4-4fdf-9d5b-352db9b6f2cf'
PYSPYT_2SECRETS = 'fd2157cf-a16e-4623-a414-95cdb248d8c8'
PYSPYT_PORTO_LAYER = '5bba80d2-2117-43ca-83af-9727f1c66871'
SOLOMON_TO_YT = '1ddedfcd-f81b-4582-a1bd-0d19b14214d3'
YT_TO_CLICKHOUSE = 'd9f314f8-6b96-4035-977a-020204a775ca'
CLICKHOUSE_QUERY = '155ded0d-13d9-40f9-80b0-bfe8ed059599'
SOLOMON_CPU_METRICS = '6b8541aa-3b2d-4738-92f4-4ff188c60716'

# @todo refactor for builder pattern

Data = str
FileName = str


def get_default_spark_op(operation_id: str):
    spark_op = vh.op(id=operation_id)
    spark_op = spark_op.partial(
        spyt_proxy=SPYT_YT_PROXY,
        spyt_discovery_path=SPYT_DISCOVERY_DIR,
        spyt_yt_token=YT_TOKEN,
        spyt_deploy_mode="cluster",
        spyt_py_files=SPARK_DEFAULT_PY_FILES,
        spyt_spark_conf=SPARK_DEFAULT_CONFIG,
    )
    return spark_op


def get_default_spark_op_pg(operation_id: str):
    spark_op = get_default_spark_op(operation_id)
    spark_op = spark_op.partial(
        spyt_driver_args=[
            f"--pg_user {PG_USER}",
            f"--pg_database {PG_DATABASE}",
            f"--pg_host {PG_HOST}",
            f"--pg_port {PG_PORT}",
        ],
        spyt_spark_conf=SPARK_DEFAULT_CONFIG + [
            f"spark.jars={SPYT_JARS_YT_ROOT}/postgresql-42.2.11.jar",
        ]
    )
    return spark_op


def get_default_spark_op_pg_1secret(operation_id: str):
    spark_op = get_default_spark_op_pg(operation_id)
    spark_op = spark_op.partial(
        spyt_secret=PG_PASSWORD_SECRET_NAME,
    )
    return spark_op


def get_default_spark_op_yav(operation_id: str):
    spark_op = get_default_spark_op(operation_id)
    spark_op = spark_op.partial(
        spyt_py_driver_args=[
            f"--pg_user {PG_USER}",
            f"--pg_database {PG_DATABASE}",
            f"--pg_host {PG_HOST}",
            f"--pg_port {PG_PORT}",
        ],
        spyt_secret_py_driver_arg=YAV_OAUTH_SECRET_NAME,
        spyt_spark_conf=SPARK_DEFAULT_CONFIG + [
            f"spark.jars={SPYT_JARS_YT_ROOT}/postgresql-42.2.11.jar",
        ]
    )
    return spark_op


def get_default_spark_op_pg_2secrets(operation_id: str):
    spark_op = get_default_spark_op(operation_id)
    spark_op = spark_op.partial(
        spyt_py_driver_args=[
            f"--pg_user {PG_USER}",
            f"--pg_database {PG_DATABASE}",
            f"--pg_host {PG_HOST}",
            f"--pg_port {PG_PORT}",
        ],
        secret1=PG_PASSWORD_SECRET_NAME,
        secret2=YT_OAUTH_SECRET_NAME,
        spyt_spark_conf=SPARK_DEFAULT_CONFIG + [
            f"spark.jars={SPYT_JARS_YT_ROOT}/postgresql-42.2.11.jar",
        ]
    )
    return spark_op


def get_default_pg_op():
    op_pg_exec = vh.op(id="a24293d6-54be-48cb-92d0-2cdccd5bf60d")
    return op_pg_exec.partial(
        host="cloud-dwh-pg-host-prod",
        port="cloud-dwh-pg-port-prod",
        dbname="cloud_dwh",
        user="etl-user",
        password="cloud-dwh-pg-etl-password-prod",
    )


def create_tar_archive(files: List[Tuple[FileName, Data]], name: Optional[str] = None, **options):
    inputs = {}

    for i, (filename, data) in enumerate(files):
        inputs['file' + str(i)] = data
        options['name' + str(i)] = filename

    create_tar = vh.op(id=CREATE_TAR_10)

    return create_tar(
        _name=name,
        _inputs=inputs,
        _options=options
    )


def get_yql_utils_library_archive_op(include_sql_files: List[str] = None):
    from library.python import resource  # noqa

    common_sql_utils_files = [
        'yql/utils/datetime.sql',
        'yql/utils/numbers.sql',
        'yql/utils/tables.sql',
        'yql/utils/helpers.sql',
        'yql/utils/currency.sql',
    ]

    archive_op_files_input = []
    for sql_file in set(common_sql_utils_files).union(include_sql_files or []):
        file_name = os.path.basename(sql_file)
        text_data = (file_name, vh.data_from_str(
            content=resource.find(sql_file), name=file_name))
        archive_op_files_input.append(text_data)

    archive_op = create_tar_archive(
        name='YQL utils', files=archive_op_files_input)

    return archive_op


def get_disable_cache_op():
    disable_cache_op = vh.op(id=GET_TIMESTAMP_NOW)
    disable_cache_op_result = disable_cache_op(
        _name='Disable cache',
        _options={
            'timestamp_type': 'string',
            'key_name': 'timestamp',
            'format': '"%d.%m.%Y %H:%M:%S"'
        },
    )

    return disable_cache_op_result


def get_default_get_mr_table_op(config: DeployConfig):
    get_mr_table_op = vh.op(id=GET_MR_TABLE)
    return get_mr_table_op.partial(
        _options={
            'cluster': config.yt_cluster,
            'yt-token': config.yt_token,
        },
    )


def get_default_get_mr_directory_op(config: DeployConfig):
    get_mr_table_op = vh.op(id=GET_MR_DIRECTORY)
    return get_mr_table_op.partial(
        _options={
            'cluster': config.yt_cluster,
            'yt-token': config.yt_token,
        },
    )


def get_default_yql_op(config: DeployConfig, disable_cache_op_result=None):
    yql_op = vh.op(id=YQL_1)
    return yql_op.partial(
        _options={
            'mr-default-cluster': config.yt_cluster,
            'mr-account': config.mr_account,
            'yt-token': config.yt_token,
            'yql-token': config.yql_token,
            'mr-output-path': config.yt_query_cache_path,
            'use_account_tmp': True,
            'yt-owners': config.yt_owners,
            'ttl': config.ttl,
            'yt-pool': config.yt_pool,
            'timestamp': vh.OptionExpr('${datetime.timestamp}'),
        },
        _dynamic_options=disable_cache_op_result or [],
    )


def get_default_yql_2_inputs_op(config: DeployConfig, disable_cache_op_result=None):
    yql_op = vh.op(id=YQL_2)
    return yql_op.partial(
        _options={
            'mr-default-cluster': config.yt_cluster,
            'mr-account': config.mr_account,
            'yt-token': config.yt_token,
            'yql-token': config.yql_token,
            'mr-output-path': config.yt_query_cache_path,
            'use_account_tmp': True,
            'yt-owners': config.yt_owners,
            'ttl': config.ttl,
            'yt-pool': config.yt_pool,
            'timestamp': vh.OptionExpr('${datetime.timestamp}'),
        },
        _dynamic_options=disable_cache_op_result or [],
    )


def get_default_yql_with_json_output_op(config: DeployConfig):
    yql_op = vh.op(id=YQL_WITH_JSON_OUTPUT)
    return yql_op.partial(
        _options={
            'encryptedOauthToken': config.yql_token,
            'ytTokenSecret': config.yt_token,
            'writeShareIdToOutput': True
        }
    )


def get_default_solomon_to_yt_op(config: DeployConfig):
    yql_op = vh.op(id=SOLOMON_TO_YT)
    return yql_op.partial(
        _options={
            'solomon-token': config.solomon_token,
            'solomon-page-size': datetimeutils.DAY,
            'solomon-now-lag': 10 * datetimeutils.MINUTE,
            'yt-token': config.yt_token,
            'debug-logging': True,
        },
    )


def get_default_solomon_cpu_metrics_op(config: DeployConfig):
    yql_op = vh.op(id=SOLOMON_CPU_METRICS)
    return yql_op.partial(
        _options={
            'solomon-token': config.solomon_token,
            'solomon-page-size': datetimeutils.DAY,
            'solomon-now-lag': 10 * datetimeutils.MINUTE,
            'yt-token': config.yt_token,
            'debug-logging': True,
        },
    )


def get_default_copy_mr_table_op(config: DeployConfig):
    copy_table_op = vh.op(id=MR_COPY_TABLE)
    return copy_table_op.partial(
        _options={
            'dst-cluster': config.yt_cluster,
            'yt-token': config.yt_token,
            'force': True,
            'mr-account': config.mr_account,
        },
    )


def get_default_mr_drop_op(config: DeployConfig):
    mr_drop_op = vh.op(id=MR_DROP)
    return mr_drop_op.partial(
        _options={
            'yt-token': config.yt_token,
        },
    )


def get_default_yt_to_clickhouse_op(config: DeployConfig):
    yt_to_clickhouse_op = vh.op(id=YT_TO_CLICKHOUSE)
    return yt_to_clickhouse_op.partial(
        _options={
            'mdb_oauth_token': config.mdb_token,
            'yt_token': config.yt_token,
            'yt_pool': config.yt_pool,
            'ch_rewrite_all_table': True,
            'use_tm': False,
            'ttl': 1800,  # enough?
            'mdb_randomize_ch_host': True,

        }
    )


def get_default_clickhouse_query_op(config: DeployConfig):
    clickhouse_query = vh.op(id=CLICKHOUSE_QUERY)
    return clickhouse_query


def get_default_single_option_to_text_output_op(config: DeployConfig):
    return vh.op(id=SINGLE_OPTION_TO_TEXT_OUTPUT)


def get_default_single_option_to_json_output_op(config: DeployConfig):
    return vh.op(id=SINGLE_OPTION_TO_JSON_OUTPUT)


def get_default_python_to_tsv_json_text_op(config: DeployConfig):
    return vh.op(id=PYTHON_SCRIPT_TO_TSV_JSON_TEXT)


def get_default_python_executor_op(config: DeployConfig, disable_cache_op_result=None):
    python_op = vh.op(id=PYTHON_EXECUTOR)
    return python_op.partial(
        _dynamic_options=disable_cache_op_result or []
    )


def get_secret_as_file_op(config: DeployConfig, secret: str):
    secret_to_file = vh.op(id=SECRET_TO_FILE)
    return secret_to_file.partial(
        _options={
            'secret_option': secret
        })


def get_push_to_solomon_op(config: DeployConfig):
    op = vh.op(id=PUSH_JSON_TO_SOLOMON)
    return op.partial(
        _options={
            'token': config.solomon_token,
            'project': 'yc-dwh',
            'cluster': config.solomon_cluster,
            'service': 'nirvana',
        },
    )


def get_bool_param_to_optional_output_op(bool: bool):
    op = vh.op(id=BOOL_PARAM_TO_OPTIONAL_OUTPUT)
    return op.partial(
        _options={
            "create_output": bool
        }
    )

import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op_pg
from cloud.dwh.nirvana.vh.config.base import PG_PASSWORD_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "ODS Billing YT to PG"
WORKFLOW_SCHEDULE = "0 0 */6 * * ? *"
SOURCE_SYSTEM_NAME = "billing"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        # load billing accounts to stg
        spark_op = get_default_spark_op_pg(PYSPYT)
        spark_op(
            _name="Load billing accounts",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/billing/billing_account.py",
            spyt_secret=PG_PASSWORD_SECRET_NAME
        )

    # run_config["workflow_guid"] = "bb959512-382f-489e-adf9-607c154cf15d"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["ods", "billing"]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}ods/pg/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

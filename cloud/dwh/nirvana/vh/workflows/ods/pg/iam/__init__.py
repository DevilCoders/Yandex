import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op_pg
from cloud.dwh.nirvana.vh.config.base import PG_PASSWORD_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "IAM pg ods"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"
SOURCE_SYSTEM_NAME = "iam"

CLOUD_OWNERS_FOLDER = "//home/cloud_analytics/import/iam/cloud_owners/1h"
CLOUD_OWNERS_LATEST = CLOUD_OWNERS_FOLDER + "/latest"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        op_spark = get_default_spark_op_pg(PYSPYT)
        res0 = op_spark(
            _name="Spark Cloud Creators YT -> PG",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/iam/cloud_creator.py",
            spyt_secret=PG_PASSWORD_SECRET_NAME
        )

        with vh.wait_for(res0):
            # load cloud creator
            with open(SQL_BASE_DIR_PG / "ods" / "iam_cloud_creator.sql") as f:
                cloud_creator = f.read()
            op_cloud_creator = get_default_pg_op()
            op_cloud_creator(
                _name="Load ods_iam_cloud_creator",
                request=cloud_creator,
            )

            # load cloud
            with open(SQL_BASE_DIR_PG / "ods" / "iam_cloud.sql") as f:
                cloud = f.read()
            op_cloud = get_default_pg_op()
            op_cloud(
                _name="Load ods_iam_cloud",
                request=cloud,
            )

    # run_config["workflow_guid"] = "262db0ef-e9d3-4330-9260-af137da74444"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["raw", SOURCE_SYSTEM_NAME],
        workflow_guid="c3e319bc-481f-4430-89dd-681c1a066bb3"
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}ods/pg/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

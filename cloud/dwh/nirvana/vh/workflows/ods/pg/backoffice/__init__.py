import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op_pg
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG
from cloud.dwh.nirvana.vh.config.base import YAV_OAUTH_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "ODS Backoffice YT to PG"
WORKFLOW_SCHEDULE = "0 30 */4 * * ? *"
SOURCE_SYSTEM_NAME = "backoffice"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        # load applications
        op_stg_applications = get_default_spark_op_pg(PYSPYT)
        res0 = op_stg_applications(
            _name="Load stg applications",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/{SOURCE_SYSTEM_NAME}/application.py",
            spyt_secret=YAV_OAUTH_SECRET_NAME
        )

        with vh.wait_for(res0):
            with open(SQL_BASE_DIR_PG / "ods" / "bcf_application.sql") as f:
                bcf_application = f.read()
            op_ods_application = get_default_pg_op()
            op_ods_application(
                _name="Load ods_bcf_application",
                request=bcf_application,
            )

        # load events
        op_events = get_default_spark_op_pg(PYSPYT)
        op_events(
            _name="Load events",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/{SOURCE_SYSTEM_NAME}/event.py",
            spyt_secret=YAV_OAUTH_SECRET_NAME
        )

        # load participants
        op_participants = get_default_spark_op_pg(PYSPYT)
        res1 = op_participants(
            _name="Load participants",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/ods/pg/import/{SOURCE_SYSTEM_NAME}/participant.py",
            spyt_secret=YAV_OAUTH_SECRET_NAME
        )

        with vh.wait_for(res0, res1):
            with open(SQL_BASE_DIR_PG / "ods" / "bcf_applicant.sql") as f:
                bcf_application = f.read()
            op_pg_applicant_ods = get_default_pg_op()
            op_pg_applicant_ods(
                _name="Load ods_bcf_applicant",
                request=bcf_application,
            )

    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["ods", SOURCE_SYSTEM_NAME],
        workflow_guid="fbd87aa2-a4ab-44d2-b08d-6d996f973633"
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}ods/pg/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE,
        retries=10,
    )

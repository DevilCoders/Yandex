import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op
from cloud.dwh.nirvana.vh.config.base import PG_DATABASE
from cloud.dwh.nirvana.vh.config.base import PG_HOST
from cloud.dwh.nirvana.vh.config.base import PG_PASSWORD_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import PG_PORT
from cloud.dwh.nirvana.vh.config.base import PG_USER
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_LOCALE_CONFIG
from cloud.dwh.nirvana.vh.config.base import SPYT_JARS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "PG export crm_lead_event_attendance"
WORKFLOW_SCHEDULE = "0 0 6 * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        with open(SQL_BASE_DIR_PG / "export" / "crm_lead_event_attendance.sql") as f:
            sql = f.read()
        op_pg_stg = get_default_pg_op()
        res0 = op_pg_stg(
            _name="Load lead_event_attendance",
            request=sql,
        )
        with vh.wait_for(res0):
            spark_op = get_default_spark_op(PYSPYT)
            spark_op(
                _name="Export lead_event_attendance",
                spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/raw/yt/common/rdb_to_yt_full.py",
                retries_on_job_failure=3,
                spyt_driver_args=[
                    f"--user {PG_USER}",
                    f"--database {PG_DATABASE}",
                    f"--host {PG_HOST}",
                    f"--port {PG_PORT}",
                    "--driver org.postgresql.Driver",
                    "--source_table export.v_crm_lead_event_attendance",
                    "--overwrite",
                    "--yt_path //home/cloud_analytics/export/crm/lead_event_attendance"
                ],
                spyt_secret=PG_PASSWORD_SECRET_NAME,
                spyt_spark_conf=SPARK_DEFAULT_LOCALE_CONFIG + [
                    "spark.executor.instances=1",
                    "spark.executor.memory=2G",
                    "spark.executor.cores=2",
                    "spark.cores.max=3",
                    f"spark.jars={SPYT_JARS_YT_ROOT}/postgresql-42.2.11.jar",
                ]
            )

    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["crm", "forms", "export"]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}export/pg/forms/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

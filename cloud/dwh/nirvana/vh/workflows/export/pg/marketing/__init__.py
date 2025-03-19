import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op
from cloud.dwh.nirvana.vh.config.base import PG_DATABASE
from cloud.dwh.nirvana.vh.config.base import PG_HOST
from cloud.dwh.nirvana.vh.config.base import PG_PASSWORD_SECRET_NAME
from cloud.dwh.nirvana.vh.config.base import PG_PORT
from cloud.dwh.nirvana.vh.config.base import PG_USER
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_LOCALE_CONFIG
from cloud.dwh.nirvana.vh.config.base import SPYT_JARS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import YT_TOKEN
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "PG export marketing"
WORKFLOW_SCHEDULE = "0 0 6 * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        yt_token = vh.add_global_option('yt_token', 'secret', default=YT_TOKEN)

        def mr_path_op():
            op = vh.op(id="6ef6b6f1-30c4-4115-b98c-1ca323b50ac0")
            return op.partial(yt_token=yt_token, cluster="hahn")

        spark_op = get_default_spark_op(PYSPYT)
        tables = ["export.v_marketing_dim_lead", "export.v_marketing_dim_event",
                  "export.v_marketing_fct_lead_event_attendance"]
        tables_str = ",".join(tables)
        yt_paths = ",".join(["//home/cloud_analytics/dwh/cdm/" + table.replace("export.v_marketing_", "")
                             for table in tables])
        res0 = spark_op(
            _name="Export lead_event_attendance",
            spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/raw/yt/common/rdb_to_yt_full.py",
            retries_on_job_failure=3,
            spyt_driver_args=[
                f"--user {PG_USER}",
                f"--database {PG_DATABASE}",
                f"--host {PG_HOST}",
                f"--port {PG_PORT}",
                "--driver org.postgresql.Driver",
                f"--source_table {tables_str}",
                "--overwrite",
                f"--yt_path {yt_paths}"
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

        with vh.wait_for(res0):
            mr_path_lead = mr_path_op()
            mr_path_lead(table="//home/cloud_analytics/dwh/cdm/dim_lead")

            mr_path_event = mr_path_op()
            mr_path_event(table="//home/cloud_analytics/dwh/cdm/dim_event")

            mr_path_lea = mr_path_op()
            mr_path_lea(table="//home/cloud_analytics/dwh/cdm/fct_lead_event_attendance")

    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["marketing", "pg", "export"],
        workflow_guid="651d7024-5fab-4496-acd2-6f850b01abc8"
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}export/pg/marketing/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

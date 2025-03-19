import vh

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG

WORKFLOW_LABEL = "CDM dim_event generation"
WORKFLOW_SCHEDULE = "0 */40 * * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        with open(SQL_BASE_DIR_PG / "cdm" / "dim_event_bcf.sql") as f:
            sql = f.read()
        op_pg_stg = get_default_pg_op()
        res0 = op_pg_stg(
            _name="Enrich dim_event from Backoffice",
            request=sql,
        )
        with vh.wait_for(res0):
            with open(SQL_BASE_DIR_PG / "cdm" / "fct_lead_event_attendance.sql") as f:
                sql = f.read()
            op_pg_stg = get_default_pg_op()
            op_pg_stg(
                _name="Update fct_lead_event_attendance",
                request=sql,
            )

    # run_config["workflow_guid"] = "bb959512-382f-489e-adf9-607c154cf15d"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["cdm", "dim_event", "fct_lead_event_attendance"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/pg/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

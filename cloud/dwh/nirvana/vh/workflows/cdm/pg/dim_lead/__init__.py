import vh

from cloud.dwh.nirvana.config import DeployContext
from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG

WORKFLOW_LABEL = "CDM dim_lead generation"
WORKFLOW_SCHEDULE = "0 */30 * * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        with open(SQL_BASE_DIR_PG / "cdm" / "dim_lead_iam.sql") as f:
            sql = f.read()
        op_pg_stg = get_default_pg_op()
        res0 = op_pg_stg(
            _name="Enrich dim_lead from IAM",
            request=sql,
        )
        with vh.wait_for(res0):
            with open(SQL_BASE_DIR_PG / "cdm" / "dim_lead_bcf.sql") as f:
                sql = f.read()
            op_pg_stg = get_default_pg_op()
            res1 = op_pg_stg(
                _name="Enrich dim_lead from Backoffice",
                request=sql,
            )
        with vh.wait_for(res1):
            with open(SQL_BASE_DIR_PG / "cdm" / "dim_lead_forms.sql") as f:
                sql = f.read()
            op_pg_stg = get_default_pg_op()
            res2 = op_pg_stg(
                _name="Enrich dim_lead from Y.Forms",
                request=sql,
            )
        with vh.wait_for(res2):
            with open(SQL_BASE_DIR_PG / "cdm" / "dim_lead_site.sql") as f:
                sql = f.read()
            op_pg_stg = get_default_pg_op()
            res2 = op_pg_stg(
                _name="Enrich dim_lead from Site",
                request=sql,
            )

        with vh.wait_for(res2):
            with open(SQL_BASE_DIR_PG / "cdm" / "dim_lead_other.sql") as f:
                sql = f.read()
            op_pg_stg = get_default_pg_op()
            op_pg_stg(
                _name="Enrich dim_lead from other sources",
                request=sql,
            )

    # run_config["workflow_guid"] = "bb959512-382f-489e-adf9-607c154cf15d"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["cdm", "dim_lead"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}cdm/pg/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

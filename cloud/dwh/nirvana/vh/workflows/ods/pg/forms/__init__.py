import vh

from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "ODS Forms Registrations PG RAW -> ODS"
WORKFLOW_SCHEDULE = "0 */30 * * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        with open(SQL_BASE_DIR_PG / "stg" / "frm_application.sql") as f:
            sql_stg = f.read()
        op_pg_stg = get_default_pg_op()
        res0 = op_pg_stg(
            _name="Load forms applications to stg",
            request=sql_stg,
        )
        with vh.wait_for(res0):
            with open(SQL_BASE_DIR_PG / "ods" / "frm_applicant.sql") as f:
                sql_applicant = f.read()
            op_pg_applicant_ods = get_default_pg_op()
            op_pg_applicant_ods(
                _name="Load ods_frm_applicant",
                request=sql_applicant,
            )

            with open(SQL_BASE_DIR_PG / "ods" / "frm_application.sql") as f:
                sql_application = f.read()
            op_pg_application_ods = get_default_pg_op()
            op_pg_application_ods(
                _name="Load ods_frm_application",
                request=sql_application,
            )

    # run_config["workflow_guid"] = "bb959512-382f-489e-adf9-607c154cf15d"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["ods", "forms"]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}ods/pg/forms/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

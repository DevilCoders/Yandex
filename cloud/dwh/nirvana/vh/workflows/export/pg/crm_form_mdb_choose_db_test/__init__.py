from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "PG export crm_form_mdb_choose_db_test"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"


def main(ctx: DeployConfig):
    run_config = dict(
        workflow_guid="8adfa0e7-e226-496c-825d-8f7d2673705f",
        label=WORKFLOW_LABEL,
        workflow_tags=["crm", "forms", "export"]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}export/pg/forms/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=None,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

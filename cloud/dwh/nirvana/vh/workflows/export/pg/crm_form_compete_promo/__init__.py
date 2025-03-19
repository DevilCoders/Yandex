from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "PG export crm_form_compete_promo"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"


def main(ctx: DeployConfig):
    run_config = dict(
        workflow_guid="c5e9d153-6296-438c-9b41-3529113593db",
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

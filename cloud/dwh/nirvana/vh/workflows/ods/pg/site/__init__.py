import vh

from cloud.dwh.nirvana.vh.common.operations import get_default_pg_op
from cloud.dwh.nirvana.vh.config.base import SQL_BASE_DIR_PG
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "SITE pg ods"
WORKFLOW_SCHEDULE = "0 0 * * * ? *"
SOURCE_SYSTEM_NAME = "site"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        # load cloud
        with open(SQL_BASE_DIR_PG / "ods" / "site_news_subscribed_email.sql") as f:
            subscribed_email = f.read()
        op_cloud = get_default_pg_op()
        op_cloud(
            _name="Load site_news_subscribed_email",
            request=subscribed_email,
        )

    # run_config["workflow_guid"] = "262db0ef-e9d3-4330-9260-af137da74444"
    run_config = dict(
        label=WORKFLOW_LABEL,
        workflow_tags=["raw", SOURCE_SYSTEM_NAME]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}ods/pg/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_LOCALE_CONFIG
from cloud.dwh.nirvana.vh.config.base import SPYT_JARS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "RAW CRM MYSQL to YT"
WORKFLOW_SCHEDULE = "0 */30 * * * ? *"

SOURCE_SYSTEM_NAME = "crm"
DB_USER = 'cloud8'
DB_HOST = 'c-mdb8t5pqa6cptk82ukmc.ro.db.yandex.net'
DB_PORT = '3306'
DB_DRIVER = 'com.mysql.jdbc.Driver'
DB_DATABASE = 'cloud8'
DB_PASSWORD_SECRET_NAME = 'yc-crm-dwh-password-prod'

TABLES = [
    "accountroles", "accountroles_audit", "accounts", "accounts_audit", "accounts_contacts", "accounts_opportunities",
    "billingaccounts", "calls", "calls_audit", "contacts", "contacts_audit", "dimensions", "dimensions_bean_rel",
    "dimensionscategories", "email_addr_bean_rel", "email_addresses", "leads", "leads_audit", "leads_billing_accounts",
    "notes", "opportunities", "opportunities_audit", "opportunities_contacts", "plans", "plans_audit",
    "product_categories", "product_templates", "product_templates_audit", "revenue_line_items",
    "revenue_line_items_audit", "segments", "tag_bean_rel", "tags", "tasks", "tasks_audit", "team_sets_teams",
    "teams", "users",
]


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        for table in TABLES:
            target_yt_table = f'//home/cloud_analytics/dwh/raw/{SOURCE_SYSTEM_NAME}/{table}'

            spark_op = get_default_spark_op(PYSPYT)
            spark_op(
                _name=f"Load {table}",
                spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/raw/yt/common/rdb_to_yt_full.py",
                retries_on_job_failure=3,
                spyt_driver_args=[
                    f"--user {DB_USER}",
                    f"--database {DB_DATABASE}",
                    f"--host {DB_HOST}",
                    f"--port {DB_PORT}",
                    f"--driver {DB_DRIVER}",
                    f"--source_table {table}",
                    f"--yt_path {target_yt_table}",
                ],
                spyt_secret=DB_PASSWORD_SECRET_NAME,
                spyt_spark_conf=SPARK_DEFAULT_LOCALE_CONFIG + [
                    "spark.executor.instances=1",
                    "spark.executor.memory=8G",
                    "spark.executor.cores=2",
                    "spark.cores.max=3",
                    f"spark.jars={SPYT_JARS_YT_ROOT}/mysql-connector-java-8.0.15.jar",
                ]
            )

    run_config = dict(
        label=WORKFLOW_LABEL,
        max_concurrent_tasks=5,
        workflow_tags=["raw", SOURCE_SYSTEM_NAME]
    )
    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}raw/yt/{SOURCE_SYSTEM_NAME}/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

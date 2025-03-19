import vh

from cloud.dwh.nirvana.vh.common.operations import PYSPYT
from cloud.dwh.nirvana.vh.common.operations import get_default_spark_op
from cloud.dwh.nirvana.vh.config.base import SPARK_DEFAULT_LOCALE_CONFIG
from cloud.dwh.nirvana.vh.config.base import SPYT_JARS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import SPYT_JOBS_YT_ROOT
from cloud.dwh.nirvana.vh.config.base import BaseDeployConfig as DeployConfig
from cloud.dwh.nirvana.config import DeployContext

WORKFLOW_LABEL = "RAW Backoffice PG to YT"
WORKFLOW_SCHEDULE = "0 0 */4 * * ? *"


def main(ctx: DeployConfig):
    with vh.Graph() as g:
        events_user = 'dwh'
        events_host = 'c-mdbc9bedenblo9descuj.rw.db.yandex.net'
        events_port = '6432'
        events_driver = 'org.postgresql.Driver'
        events_database = 'db_prod'
        events_password_secret_name = 'yc-storage-dwh-password-prod'
        table_list = ["applications", "blog_post_locales", "blog_posts", "blog_posts_likes", "blog_posts_tags",
                      "blog_tag_locales", "blog_tags", "case_categories", "cases", "cases_services", "cities",
                      "event_reminders", "event_speakers", "events", "events_services", "features", "features_services",
                      "incident_comments", "incidents", "incidents_services", "incidents_zones", "knex_migrations",
                      "knex_migrations_lock", "page_locales", "page_versions", "pages", "participants", "places",
                      "services", "services_zones", "speakers", "speeches", "speeches_speakers", "votes", "zones"]
        for table in table_list:
            target_yt_table = f'//home/cloud_analytics/dwh/raw/backoffice/{table}'
            spark_op = get_default_spark_op(PYSPYT)
            spark_op(
                _name=f"Load {table}",
                spyt_driver_file=f"{SPYT_JOBS_YT_ROOT}/raw/yt/common/rdb_to_yt_full.py",
                retries_on_job_failure=3,
                spyt_driver_args=[
                    f"--user {events_user}",
                    f"--database {events_database}",
                    f"--host {events_host}",
                    f"--port {events_port}",
                    f"--driver {events_driver}",
                    f"--source_table {table}",
                    f"--yt_path {target_yt_table}",
                ],
                spyt_secret=events_password_secret_name,
                spyt_spark_conf=SPARK_DEFAULT_LOCALE_CONFIG + [
                    "spark.executor.instances=1",
                    "spark.executor.memory=8G",
                    "spark.executor.cores=2",
                    "spark.cores.max=3",
                    f"spark.jars={SPYT_JARS_YT_ROOT}/postgresql-42.2.11.jar",
                ]
            )

    run_config = dict(
        workflow_guid="db194e70-30e3-4fb5-8c27-4379ed5103cd",
        label=WORKFLOW_LABEL,
        max_concurrent_tasks=2,
        workflow_tags=["raw", "backoffice"]
    )

    reaction_name = WORKFLOW_LABEL.lower().replace(" ", "_")
    reaction_path = f'{ctx.reactor_path_prefix}raw/yt/backoffice/{reaction_name}'

    return DeployContext(
        run_config=run_config,
        graph=g,
        reaction_path=reaction_path,
        schedule=WORKFLOW_SCHEDULE
    )

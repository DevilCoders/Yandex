import logging.config
import os
import click
from clan_tools.logging.logger import default_log_config
from clan_tools.utils.spark import SPARK_CONF_SMALL
from pyspark.sql.functions import col
from spyt import spark_session
# if you want to run local use spark-submit or something


os.environ["JAVA_HOME"] = "/usr/local/jdk-11"
os.environ["ARROW_PRE_0_15_IPC_FORMAT"] = "0"


logging.config.dictConfig(default_log_config)
logger = logging.getLogger(__name__)


@click.command()
@click.option('--leads_path', default="//home/cloud_analytics/ml/tracker_scoring/leads_by_org_path")
@click.option('--passport_import_path', default="//home/cloud_analytics/import/passport/puids")
@click.option('--iam_organizations_path', default="//home/cloud-dwh/data/prod/ods/iam/organizations")
@click.option('--iam_roles_path', default="//home/cloud-dwh/data/prod/ods/iam/role_bindings")
@click.option('--iam_puid_path', default="//home/cloud-dwh/data/prod/ods/iam/passport_users")
def main(leads_path: str, passport_import_path: str, iam_organizations_path: str, iam_roles_path: str, iam_puid_path: str):

    with spark_session(yt_proxy="hahn", spark_conf_args=SPARK_CONF_SMALL, driver_memory='2G') as spark:

        big_orgs = (
            spark.read.yt(iam_organizations_path)
            .filter(col('size').isin(["l", "xl"]))
            .select(col('organization_id').alias('resource_id'), 'display_name', 'size')
        )

        passport_iam_matching = (
            spark.read.yt(iam_puid_path)
            .select(col('iam_uid').alias('subject_id'), 'passport_uid')
        )

        leads_iam_data = (
            spark.read.yt(iam_roles_path)
            .join(big_orgs, how='right', on='resource_id')
            .filter((~col('is_system')) & (col('role_slug') == "organization-manager.organizations.owner"))
            .join(passport_iam_matching, how='left', on='subject_id')
            .filter(col('passport_uid').isNotNull())
            .select('resource_id', 'display_name', 'size', 'subject_id', 'role_slug', 'passport_uid')
        )

        only_new_leads = (
            leads_iam_data
            .join(
                spark.read.yt(leads_path).select('passport_uid'),
                how='left_anti', on='passport_uid')
            .select('resource_id', 'display_name', 'size', 'subject_id', 'role_slug', 'passport_uid').distinct()
        )

        puids_to_passport = (
            leads_iam_data.select(col('passport_uid').alias('puid')).distinct()
            .join(spark.read.yt(passport_import_path), how='left_anti', on='puid')
        )

        only_new_leads.coalesce(1).write.yt(leads_path, mode='append')
        puids_to_passport.coalesce(1).write.yt(passport_import_path, mode='append')


if __name__ == '__main__':
    main()

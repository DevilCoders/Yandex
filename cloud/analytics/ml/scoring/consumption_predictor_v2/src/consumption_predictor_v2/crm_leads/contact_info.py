from pyspark.sql.functions import col
from clan_tools.data_adapters.crm.CRMHistoricalDataAdapter import CRMHistoricalDataAdapter


def filtered_crm_ba(yt_adapter, spark, lead_source_crm):
    historical_data_adapter = CRMHistoricalDataAdapter(yt_adapter, spark)
    crm_historical_data = historical_data_adapter.historical_preds().cache()
    leads_filtered = crm_historical_data.filter(col("lead_source_crm")==lead_source_crm)
    return leads_filtered


def get_ba_contact_info(spark):
    """
    DEPRECATED IN FUTURE
    """
    cube = "//home/cloud_analytics/cubes/acquisition_cube/cube"
    bindings = "//home/cloud-dwh/data/prod/ods/billing/service_instance_bindings"
    timezones = "//home/cloud_analytics/import/iam/cloud_owners/1h/latest"
    ba_contact_info_main = (
        spark.read.yt(cube)
        .filter(col('ba_usage_status') != 'service')
        .filter((col('segment') == 'Mass') | (col('segment') == 'Medium'))
        .filter(col("email")!='fake@fake.ru')
        .select(
            "billing_account_id",
            col("account_name").alias("client_name"),
            "phone",
            "email",
            "first_name",
            "last_name"
        )
        .distinct()
    )
    spdf_binds = (
        spark.read.yt(bindings)
        .filter(col("service_instance_type")=="cloud")
        .select("billing_account_id", col("service_instance_id").alias("cloud_id"))
    )
    cloud_timezones = (
        spark.read.yt(timezones)
        .select("cloud_id", "timezone")
    )
    ba_contact_info = (
        ba_contact_info_main
        .join(spdf_binds, on="billing_account_id", how="left")
        .join(cloud_timezones, on="cloud_id", how="left")
        .cache()
    )
    return ba_contact_info

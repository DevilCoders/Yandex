USE hahn;

$format = DateTime::Format("%Y-%m-%dT00:00:00");
$to_datetime = ($str) -> {RETURN DateTime::MakeDatetime(DateTime::ParseIso8601($str)) };

$dl_daily_usage = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_daily_usage';
$crm_tags = '//home/cloud-dwh/data/prod/cdm/dm_ba_crm_tags';
$result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/dl_consumption';

DEFINE ACTION $dl_consumption_script() AS

$dl_ba_info = (
        SELECT 
        DISTINCT
            dl.billing_account_id AS billing_account_id,
            crm_account_name,
            crm_architect_current,
            crm_account_owner_current,
            segment,
            segment_current,
            billing_account_usage_status, 
            is_subaccount,
            is_var,
            $format($to_datetime(event_date)) AS event_date
        FROM $dl_daily_usage as dl
        INNER JOIN ( 
            SELECT 
            DISTINCT 
                billing_account_id,
                IF(crm_account_name!='',crm_account_name, billing_account_name) AS crm_account_name,
                architect_current AS crm_architect_current,
                IF(account_owner_current='mstrizhak' AND segment_current='Mass', 'No Account Owner', account_owner_current) AS crm_account_owner_current,
                segment,
                segment_current,
                usage_status AS billing_account_usage_status,
                is_subaccount,
                is_var,
                $format($to_datetime(date)) AS ts_date
            FROM $crm_tags 
            WHERE 1=1
                AND is_suspended_by_antifraud_current = False 
                AND $format($to_datetime(date)) >= $format($to_datetime('2020-04-14')) -- минимальная дата в логах по реквестам даталенса
                AND $format($to_datetime(date)) < $format(CurrentUtcDatetime())
            ) as consumpiton
            ON consumpiton.billing_account_id = dl.billing_account_id
                AND consumpiton.ts_date = $format($to_datetime(dl.event_date))
);

INSERT INTO $result_table_path
WITH TRUNCATE
    SELECT * FROM $dl_ba_info;

END DEFINE;

EXPORT $dl_consumption_script;

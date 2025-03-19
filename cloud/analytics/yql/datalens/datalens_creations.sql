USE hahn;
PRAGMA AnsiInForEmptyOrNullableItemsCollections;

-------------------------------------------PATH-------------------------------------------
$prod_file = '//home/cloud_analytics/data_swamp/projects/datalens/yc_prod_dl_orgs';
$logs_folder = '//home/logfeller/logs/dataui-prod-nodejs-log/1d/';
$organizations = '//home/cloud-dwh/data/prod/ods/iam/organizations';
$events = '//home/cloud-dwh/data/prod/cdm/dm_events';
$ba_org_dict = '//home/cloud_analytics/dictionaries/ids/ba_cloud_organization_dict';

$dl_result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/datalens_creations';
$org_result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/org_creations';
$mdb_result_table_path = '//home/cloud_analytics/data_swamp/projects/datalens/mdb_first_consumption';
-------------------------------------------FUNCTIONS-------------------------------------------
$parse_datetime= DateTime::Parse("%Y-%m-%dT%H:%M:%SZ");
$str_to_datetime = ($str) -> (DateTime::MakeDatetime($parse_datetime($str)));
$format_d = DateTime::Format("%Y-%m-%d");
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
$format_msk_date = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_d));

-- -------------------------------------------QUERY-------------------------------------------
DEFINE ACTION $org_counting_script() AS
----------------------------------organizations with dl (create dl date)----------------------
    $dl = (
        -- до '2022-06-29' включительно берем организации из файлика 
        SELECT
        -- substring(created_at, 0, 19) as created_at_datetime_msk, 
            substring(created_at, 0, 10) as created_at_date_msk,
            org_id                       as organization_id,
            NULL                         as user_id
        FROM $prod_file
        WHERE substring(created_at, 0, 10) <= '2022-06-29'

        UNION ALL 

        -- с '2022-06-30' из логов
        SELECT
            created_at_date_msk,
            organization_id,
            user_id
        FROM (
            SELECT 
            -- $str_to_datetime(Yson::ConvertToString(`_rest`["@fields"]["extra"]["createdAt"])) as created_at_datetime_utm,
            $format_msk_date($str_to_datetime(Yson::ConvertToString(`_rest`["@fields"]["extra"]["createdAt"]))) as created_at_date_msk,
            Yson::ConvertToString(`_rest`["@fields"]["extra"]["organizationId"]) as organization_id,
            Yson::ConvertToString(`_rest`["@fields"]["extra"]["userId"]) as user_id
        FROM RANGE($logs_folder,`2022-06-30`)
        WHERE app_name IN ('united-storage', 'datalens-charts', 'datalens-datalens') 
            AND msg like '%YC_ORG_DL_INITED%'
        )
        WHERE created_at_date_msk >= '2022-06-30'
        ) 
    ;

    INSERT INTO $dl_result_table_path WITH TRUNCATE
        select * from $dl
        order by created_at_date_msk
        ;
----------------------------------all organizations----------------------------------
    INSERT INTO $org_result_table_path WITH TRUNCATE
        SELECT 
            organization_id,
            created_at_date_msk,
            probably_fraud,
            IF(created_at_date_msk in ('2022-06-03','2022-06-04','2022-06-05','2022-06-06'), probably_fraud, False) as probably_fraud_only_june
        FROM (
            SELECT
                organization_id,
                $format_msk_date(created_at_msk) as created_at_date_msk,
                CAST(name as string) RLIKE "^organization-[a-zA-Z]+[1][0-9]{3}$" as probably_fraud,
            FROM $organizations 
        )
        ORDER BY created_at_date_msk
    ;
----------------------------------organizations with mdb (first usage date)-----------
    $mdb = (
        SELECT  
            a.billing_account_id as billing_account_id,
            first_mdb_msk_date,
            cloud_start_msk,
            cloud_end_msk,
            b.organization_id as organization_id,
            created_at_date_msk
        FROM (
            SELECT
                billing_account_id,
                min(msk_event_dt) as first_mdb_msk_date
            FROM $events
            WHERE event_type in (
                    'billing_account_first_paid_mdb_consumption',
                    'billing_account_first_trial_mdb_consumption'
                    )
            GROUP BY billing_account_id
            ) as a
        LEFT JOIN (
            SELECT DISTINCT
                billing_account_id,
                substring(cloud_start_msk, 0, 10) as cloud_start_msk,
                substring(cloud_end_msk, 0, 10) as cloud_end_msk,
                organization_id
            FROM $ba_org_dict 
            ) as b
        on a.billing_account_id = b.billing_account_id
        LEFT JOIN (
            SELECT
                organization_id,
                $format_msk_date(created_at_msk) as created_at_date_msk
            FROM $organizations 
            ) as c
            on b.organization_id = c.organization_id
        WHERE first_mdb_msk_date between cloud_start_msk and cloud_end_msk
    );


    INSERT INTO $mdb_result_table_path WITH TRUNCATE
        SELECT
            organization_id,
            first_mdb_msk_date as created_at_date_msk
        FROM $mdb
        ORDER BY created_at_date_msk
    ;

END DEFINE;

EXPORT $org_counting_script;

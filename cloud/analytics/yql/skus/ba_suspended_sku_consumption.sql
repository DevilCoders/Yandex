USE hahn;

PRAGMA AnsiInForEmptyOrNullableItemsCollections;
------------------------- PATH ------------------------------------------
$ba_history = '//home/cloud-dwh/data/prod/ods/billing/billing_accounts_history';
$dm_yc_consumption =  '//home/cloud-dwh/data/prod/cdm/dm_yc_consumption';
$billing_records = '//home/cloud-dwh/data/prod/ods/billing/billing_records/1mo/';
$sku_lazy = '//home/cloud_analytics/data_swamp/projects/skus/sku_lazy';

$billing_states_suspended_path = '//home/cloud_analytics/data_swamp/projects/skus/billing_states_suspended';
$ba_sku_susp_consumption_14d_path = '//home/cloud_analytics/data_swamp/projects/skus/ba_sku_susp_consumption_14d';
$ba_sku_susp_consumption_30d_path = '//home/cloud_analytics/data_swamp/projects/skus/ba_sku_susp_consumption_30d';
$ba_sku_susp_consumption_14d_resource_path = '//home/cloud_analytics/data_swamp/projects/skus/ba_sku_susp_consumption_14d_resource';
$ba_sku_susp_consumption_30d_resource_path = '//home/cloud_analytics/data_swamp/projects/skus/ba_sku_susp_consumption_30d_resource';
----------------------- FUNCTION ----------------------------------------
$format_d = DateTime::Format("%Y-%m-%d");
$format_dt = DateTime::Format("%Y-%m-%d %H:%M:%S");
-- convert timestamp to DateTime with timezone
$get_tz_datetime = ($ts, $tz) -> (AddTimezone(DateTime::FromSeconds(CAST($ts AS Uint32)), $tz));
-- convert timestamp to special format with timezone
$format_by_timestamp = ($ts, $tz, $fmt) -> ($fmt($get_tz_datetime($ts, $tz)));
-- convert timestamp to datetime string (e.g. 2020-12-31 03:00:00) with timezone
$msk_date = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_d));
$msk_datetime = ($ts) -> ($format_by_timestamp($ts, "Europe/Moscow", $format_dt));
----------------------- SCRIPT ----------------------------------------
DEFINE ACTION $ba_suspended_sku_consumption_script() AS

    -- берем из ba_history только строки где у ба менялся статус
    $change_state = (
        SELECT 
            billing_account_id,
            updated_at as updated_at,
            state
        FROM (
            SELECT 
                billing_account_id,
                updated_at as updated_at,
                state,
                LAG(state) over w as prev_state
            FROM $ba_history
            WINDOW w AS ( -- window specification for the named window "w" used above
                    PARTITION BY billing_account_id-- partitioning clause
                    ORDER BY updated_at ASC    -- sorting clause
                    ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
                    )
            )
        WHERE prev_state != state
    );

    -- границы периода действия статуса
    $billing_states = (
        SELECT 
            billing_account_id,
            state,
            updated_at as state_start_dt,
            LEAD(updated_at) over w as state_end_dt,
            $msk_date(updated_at + DateTime::IntervalFromDays(1)) as state_start_next_dt,
            $msk_date(updated_at + DateTime::IntervalFromDays(14)) as state_start_next14d_dt,
            $msk_date(updated_at + DateTime::IntervalFromDays(30)) as state_start_next30d_dt,
            coalesce($msk_date(LEAD(updated_at) - DateTime::IntervalFromDays(1)), CAST(CurrentUtcDate() as string)) over w as state_end_prev_dt
            FROM $change_state
            WINDOW w AS ( -- window specification for the named window "w" used above
                        PARTITION BY billing_account_id-- partitioning clause
                        ORDER BY updated_at ASC    -- sorting clause
                        ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING
                        )
    );

    -- интересует только статус suspended и период длинной хотя бы 1 полный день
    $billing_states_suspended = (
        SELECT * 
        FROM $billing_states
        WHERE 1=1
            AND state_start_next_dt <= state_end_prev_dt 
            AND state = 'suspended'
    );

    INSERT INTO $billing_states_suspended_path WITH TRUNCATE
    SELECT 
        DISTINCT
        *
        FROM $billing_states_suspended
        ORDER BY billing_account_id, state_start_next_dt
    ;

    -- потребление ба в статусе suspended (спустя 14 дней)
    $ba_sku_susp_consumption_14d = (
        SELECT 
            c.billing_account_id as billing_account_id,
            c.billing_record_total_rub_vat as billing_record_total_rub_vat,
            c.billing_record_cost_rub_vat as billing_record_cost_rub_vat,
            c.billing_record_pricing_quantity as billing_record_pricing_quantity,
            billing_record_msk_date,
            billing_account_state,
            c.sku_id as sku_id,
            c.sku_name as sku_name, 
            c.sku_lazy as prev_sku_lazy,
            sl.sku_lazy as sku_lazy,
            sl.raw_sku_lazy as raw_sku_lazy,
            sku_service_group, sku_service_name, sku_subservice_name
            FROM $dm_yc_consumption as c
            LEFT JOIN $billing_states_suspended as b
                ON c.`billing_account_id` = b.`billing_account_id`
            LEFT JOIN $sku_lazy as sl
                ON c.sku_id = sl.sku_id
            WHERE 1=1
                AND billing_record_msk_date >= '2022-01-01'
                AND c.`billing_record_msk_date` BETWEEN state_start_next14d_dt and state_end_prev_dt
    );

    -- потребление ба в статусе suspended (спустя 30 дней)
    $ba_sku_susp_consumption_30d = (
        SELECT 
            c.billing_account_id as billing_account_id,
            c.billing_record_total_rub_vat as billing_record_total_rub_vat,
            c.billing_record_cost_rub_vat as billing_record_cost_rub_vat,
            c.billing_record_pricing_quantity as billing_record_pricing_quantity,
            billing_record_msk_date,
            billing_account_state,
            c.sku_id as sku_id,
            c.sku_name as sku_name, 
            c.sku_lazy as prev_sku_lazy,
            sl.sku_lazy as sku_lazy,
            sl.raw_sku_lazy as raw_sku_lazy,
            sku_service_group, sku_service_name, sku_subservice_name
            FROM $dm_yc_consumption as c
            LEFT JOIN $billing_states_suspended as b
                ON c.`billing_account_id` = b.`billing_account_id`
            LEFT JOIN $sku_lazy as sl
                ON c.sku_id = sl.sku_id
            WHERE 1=1
                AND billing_record_msk_date >= '2022-01-01'
                AND c.`billing_record_msk_date` BETWEEN state_start_next30d_dt and state_end_prev_dt
    );


    INSERT INTO $ba_sku_susp_consumption_14d_path WITH TRUNCATE
    SELECT 
        DISTINCT
        *
        FROM $ba_sku_susp_consumption_14d
        ORDER BY billing_account_id, billing_record_msk_date
    ;

    INSERT INTO $ba_sku_susp_consumption_30d_path WITH TRUNCATE
    SELECT 
        DISTINCT
        *
        FROM $ba_sku_susp_consumption_30d
        ORDER BY billing_account_id, billing_record_msk_date
    ;

    -- биллинговые записи по ресурсам по часам, начиная с 2022-01-01
    $b_records = (
        SELECT
            billing_account_id,
            date, 	
            $msk_datetime(start_time) as start_time,
            $msk_datetime(end_time) as end_time,
            pricing_quantity,
            cost, 
            credit, 
            reward,
            currency,
            sku_id,
            cloud_id,
            folder_id,
            resource_id
        FROM RANGE($billing_records, `2022-01-01`)
    );
    
    INSERT INTO $ba_sku_susp_consumption_14d_resource_path WITH TRUNCATE
        SELECT 
            b.billing_account_id as billing_account_id,
            billing_account_state,
            billing_record_msk_date,
            start_time,
            end_time,
            billing_record_total_rub_vat,
            billing_record_cost_rub_vat,
            billing_record_pricing_quantity,
            pricing_quantity,
            cost, 
            credit, 
            reward,
            currency,
            b.sku_id as sku_id,
            b.sku_name as sku_name,  
            r.cloud_id as cloud_id,
            r.folder_id as folder_id,
            r.resource_id as resource_id,
            b.sku_lazy as prev_sku_lazy,
            sl.raw_sku_lazy as raw_sku_lazy,
            sl.sku_lazy as sku_lazy,
            sku_service_group, sku_service_name, sku_subservice_name
        FROM $ba_sku_susp_consumption_14d as b
        LEFT JOIN $b_records as r
            ON b.billing_account_id = r.billing_account_id
            AND b.sku_id = r.sku_id
            AND b.billing_record_msk_date = r.date
        LEFT JOIN $sku_lazy as sl
            ON b.sku_id = sl.sku_id
    ;

    INSERT INTO $ba_sku_susp_consumption_30d_resource_path WITH TRUNCATE
        SELECT 
            b.billing_account_id as billing_account_id,
            billing_account_state,
            billing_record_msk_date,
            start_time,
            end_time,
            billing_record_total_rub_vat,
            billing_record_cost_rub_vat,
            billing_record_pricing_quantity,
            pricing_quantity,
            cost, 
            credit, 
            reward,
            currency,
            b.sku_id as sku_id,
            b.sku_name as sku_name, 
            r.cloud_id as cloud_id,
            r.folder_id as folder_id,
            r.resource_id as resource_id,
            b.sku_lazy as prev_sku_lazy,
            sl.raw_sku_lazy as raw_sku_lazy,
            sl.sku_lazy as sku_lazy,
            sku_service_group, sku_service_name, sku_subservice_name
        FROM $ba_sku_susp_consumption_30d as b
        LEFT JOIN $b_records as r
            ON b.billing_account_id = r.billing_account_id
            AND b.sku_id = r.sku_id
            AND b.billing_record_msk_date = r.date
        LEFT JOIN $sku_lazy as sl
            ON b.sku_id = sl.sku_id
    ;

END DEFINE;

EXPORT $ba_suspended_sku_consumption_script;

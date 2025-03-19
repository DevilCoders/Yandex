use hahn;

IMPORT time SYMBOLS $parse_datetime, $parse_date,  $format_date, $dates_range;
IMPORT tables SYMBOLS $last_non_empty_table;

DEFINE SUBQUERY $lib_opt_history() AS 

    $script = @@
def fill_none(a_):
    a = [x.decode('UTF-8') for x in list(a_)]
    if set(a) == {''}:
        return a
    for i, j in enumerate(a):
        if j == '':
            if i == 0:
                a[0] = next(item for item in a if item != '')
            else:
                a[i] = a[i-1]
    return a
    @@;

    $fill_none = Python3::fill_none(
        Callable<(List<String?>)->List<String>>,

        $script
    );


    $acc_owner_history_array = (
        SELECT
            account_id,
            users.user_name as account_owner,
            $dates_range( 
                coalesce($format_date(date_from),'2018-01-01'), 
                coalesce($format_date(date_to), coalesce($format_date(CurrentUtcTimestamp() + INTERVAL('P7D')), ''))
            ) as `date` 
        FROM
            `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition1/cloud8_accountroles` as accountroles 
        LEFT JOIN 
            `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition2/cloud8_users` as users
        ON accountroles.assigned_user_id = users.id 

        WHERE 
            accountroles.deleted = 0
            and role = 'account_owner'
            and accountroles.status in ('recycled','pending_recycled','confirmed')
            AND date_from is not null

    );

    $acc_owner_history_flat = (
        SELECT
            account_id,
            account_owner,
            `date`
        FROM $acc_owner_history_array
        FLATTEN BY 
            `date`
    );

    $acc_owner_history = (
        SELECT
            account_id,
            `date`,
            AGGREGATE_LIST(account_owner)[0] as account_owner
        FROM $acc_owner_history_flat
        GROUP BY 
            account_id,
            `date`
    );

    $acc_segment_history_array = (
        SELECT
            account_id,
            name as segment,
            $dates_range( 
                coalesce($format_date(date_from),'2018-01-01'), 
                coalesce($format_date(date_to), coalesce($format_date(CurrentUtcTimestamp() + INTERVAL('P7D')), ''))
            ) as `date` 
        FROM
            `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition2/cloud8_segments`
        WHERE 
            deleted = 0
            AND date_from is not null

    );

    $acc_segment_history_flat = (
        SELECT
            account_id,
            segment,
            `date`
        FROM $acc_segment_history_array
        FLATTEN BY 
            `date`
    );

    $acc_segment_history = (
        SELECT
            account_id,
            `date`,
            AGGREGATE_LIST(segment)[0] as segment
        FROM $acc_segment_history_flat
        GROUP BY 
            account_id,
            `date`
    );


    $opp_acc = (
        SELECT 
            opportunity_id,
            MAX_BY(account_id, date_modified) as account_id 
        FROM `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition1/cloud8_accounts_opportunities`
        GROUP BY 
            opportunity_id
    );

    $opp_segment_history = (
        SELECT 
            opportunity_id,
            opp_acc.account_id as account_id,
            `date`,
            segment
        FROM $opp_acc as opp_acc 
        LEFT JOIN $acc_segment_history as acc_segment_history 
        ON opp_acc.account_id = acc_segment_history.account_id
    );


    DEFINE SUBQUERY $crm_opp_value_history($field_name) AS

        $opt_history_raw = (
            SELECT 
                parent_id as opp_id,
                before_value_string,
                after_value_string,
                date_created,
                field_name,
                $format_date(date_created) as `date`
            FROM `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition2/cloud8_opportunities_audit`
        );

        $opp_history_date = (
        SELECT 
            opp_id, 
            MIN_BY(before_value_string, date_created) AS before_value_string, 
            MAX_BY(after_value_string,date_created) AS after_value_string,
            `date`
        FROM $opt_history_raw
        WHERE field_name = $field_name
        GROUP BY opp_id, `date`
        ORDER BY opp_id, `date` DESC
        );

        $opp_value_history = (
        SELECT 
            opp_id,
            `date`,
            AGGREGATE_LIST(value)[0] as value
        FROM (
            SELECT 
                opp_id,
                `date`,
                before_value_string as value
            FROM $opp_history_date

            UNION ALL 

            SELECT 
                opp_id,
                $format_date(DateTime::MakeDatetime($parse_date(`date`)) + INTERVAL('P1D')) as `date`,
                after_value_string as value
            FROM $opp_history_date
        )
        GROUP BY 
            opp_id,
            `date`
        );

        $opp_dates_range = (
            SELECT 
                opp_id,
                `date`
            FROM (
                SELECT 
                    id as opp_id,
                    $dates_range(COALESCE($format_date(date_entered),'2018-01-01'), $format_date(CurrentUtcTimestamp())) as `date`
                FROM
                    `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition2/cloud8_opportunities`
            )
            FLATTEN BY `date`
        );

        $opp_value_history_with_null = (
            SELECT 
                opp_dates_range.opp_id as opp_id,
                opp_dates_range.`date` as `date`,
                opp_value_history.value as value 
            FROM 
                $opp_dates_range as opp_dates_range
            LEFT JOIN 
                $opp_value_history as opp_value_history
            ON 
                opp_dates_range.opp_id = opp_value_history.opp_id
                AND opp_dates_range.`date` = opp_value_history.`date`
            ORDER BY opp_id, `date` DESC 
            LIMIT 100000000000
        );

        SELECT 
            opp_id,
            a.0 as `date`,
            a.1 as value
        FROM (
            SELECT 
                opp_id,
                ListZip(
                    AGGREGATE_LIST(`date`),
                    $fill_none(AGGREGATE_LIST(COALESCE(value,'')))
                ) as a
            FROM 
                $opp_value_history_with_null
            GROUP BY opp_id
        )
        FLATTEN BY 
            a
    END DEFINE;




    $opp_history = (
    SELECT 
        sales_stage.`date` AS `date`,
        sales_stage.opp_id AS opp_id,
        sales_stage,
        amount,
        probability,
        coalesce(date_closed_month,date_closed) as date_closed
    FROM (
    SELECT 
        `date`,
        opp_id,
        value as sales_stage
    FROM $crm_opp_value_history('sales_stage')
    ) as sales_stage 
    LEFT JOIN (
    SELECT 
        `date`,
        opp_id,
        value as amount
    FROM $crm_opp_value_history('amount')   
    ) as amount
    ON 
        sales_stage.`date` = amount.`date`
        AND sales_stage.opp_id = amount.opp_id
    LEFT JOIN (
    SELECT 
        `date`,
        opp_id,
        value as probability
    FROM $crm_opp_value_history('probability')   
    ) as probability
    ON 
        sales_stage.`date` = probability.`date`
        AND sales_stage.opp_id = probability.opp_id
    LEFT JOIN (
    SELECT 
        `date`,
        opp_id,
        value as date_closed
    FROM $crm_opp_value_history('date_closed')   
    ) as date_closed
    ON 
        sales_stage.`date` = date_closed.`date`
        AND sales_stage.opp_id = date_closed.opp_id
    LEFT JOIN (
    SELECT 
        `date`,
        opp_id,
        value as date_closed_month
    FROM $crm_opp_value_history('date_closed_month')   
    ) as date_closed_month
    ON 
        sales_stage.`date` = date_closed_month.`date`
        AND sales_stage.opp_id = date_closed_month.opp_id
    );

    $opp_original = (

        SELECT DISTINCT 
            opp_id,
            opp_name,
            opp_date_ent,
            acc_id as account_id,
            acc_name as account_name,
            opp_likely as amount,
            CAST(opp_probability_percentages AS String) as probability,
            opp_sales_stage as sales_stage,
            IF(opp_date_close_by = 'close_month',opp_expected_close_month,opp_expected_close_date) as date_closed,
            CAST(non_recurring AS String) as non_recurring
        FROM `//home/cloud_analytics/import/crm/oppts/oppty_cube` as oppty_cube
        LEFT JOIN `//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/partition2/cloud8_opportunities` as opts
        ON oppty_cube.opp_id = opts.id 
    );

    $opp_original_history_array = (
        SELECT
            opp_id,
            opp_name,
            $dates_range( 
                coalesce($format_date($parse_datetime(opp_date_ent)),'2018-01-01'), 
                coalesce($format_date(CurrentUtcTimestamp()), '')
            ) as `date` ,
            account_id,
            account_name,
            amount,
            probability,
            sales_stage,
            date_closed,
            non_recurring
        FROM 
            $opp_original
    );

    $opp_original_history = (
        SELECT 
            opp_id,
            `date`,
            AGGREGATE_LIST(opp_name)[0] as opp_name,
            AGGREGATE_LIST(account_id)[0] as account_id,
            AGGREGATE_LIST(account_name)[0] as account_name,
            AGGREGATE_LIST(amount)[0] as amount,
            AGGREGATE_LIST(probability)[0] as probability,
            AGGREGATE_LIST(sales_stage)[0] as sales_stage,
            AGGREGATE_LIST(date_closed)[0] as date_closed,
            AGGREGATE_LIST(non_recurring)[0] as non_recurring
        FROM (
            SELECT 
                opp_id,
                opp_name,
                `date`,
                account_id,
                account_name,
                amount,
                probability,
                sales_stage,
                date_closed,
                non_recurring
            FROM 
                $opp_original_history_array
            FLATTEN BY 
                `date`
        )
        GROUP BY 
            opp_id, 
            `date`
    );

    $query_with_duplicates = (
    SELECT  
        opp_original_history.`date` as `date`,
        opp_original_history.opp_id as opp_id,
        opp_original_history.opp_name as opp_name,
        if(coalesce(opp_history.sales_stage,'') != '',opp_history.sales_stage, opp_original_history.sales_stage) as sales_stage,
        if(coalesce(CAST(opp_history.amount AS Double),0.0) != 0, CAST(opp_history.amount AS Double), opp_original_history.amount) as amount,
        if(opp_original_history.`date` = $format_date(CurrentUtcTimestamp()) and opp_history.probability != opp_original_history.probability, opp_original_history.probability,
            if(coalesce(opp_history.probability,'') != '',opp_history.probability, opp_original_history.probability)) as probability,
        if(coalesce(opp_history.date_closed,'') != '',opp_history.date_closed, $format_date($parse_date(opp_original_history.date_closed))) as date_closed,
        non_recurring,
        opp_original_history.account_id as account_id, 
        opp_original_history.account_name as account_name, 
        segment,
        account_owner,
        group_name,
        group_full_path
    FROM $opp_original_history as opp_original_history
    LEFT JOIN $opp_history as opp_history 
    ON 
        opp_original_history.opp_id = opp_history.opp_id
        AND opp_original_history.`date` = opp_history.`date`
    LEFT JOIN $opp_segment_history as opp_segment_history
    ON 
        opp_history.opp_id = opp_segment_history.opportunity_id
        AND opp_history.`date` = opp_segment_history.`date`
    LEFT JOIN $acc_owner_history_flat as account_owners
    ON
        opp_original_history.account_id = account_owners.account_id 
        AND opp_original_history.`date` = account_owners.`date`
    LEFT JOIN `//home/cloud_analytics/import/staff/cloud_staff/cloud_staff` as cloud_staff 
    ON account_owners.account_owner = cloud_staff.login
    );

    SELECT
        `date`,
        opp_id,
        opp_name,
        sales_stage,
        amount,
        probability,
        date_closed,
        non_recurring,
        account_id, 
        account_name, 
        segment,
        account_owner,
        group_name,
        group_full_path  
    FROM 
        $query_with_duplicates
    GROUP BY
        `date`,
        opp_id,
        opp_name,
        sales_stage,
        amount,
        probability,
        date_closed,
        non_recurring,
        account_id, 
        account_name, 
        segment,
        account_owner,
        group_name,
        group_full_path  

END DEFINE;

EXPORT $lib_opt_history;

use hahn;

$final_table_path = '//home/cloud_analytics/data_swamp/projects/oppty_product_forecast/oppty_forecast_montly_path';

$oppty_path = '//home/cloud_analytics/kulaga/oppty_cube_with_dimensions';
$new_revenue_2022_path = '//home/cloud_analytics/data_swamp/common/plans/2022/new_revenue_by_segment_service_group';
$str_to_date = ($str) -> {RETURN CAST(SUBSTRING($str, 0, 10) AS DATE)};
$toInt = ($Date) -> {RETURN CAST($Date as Int32)};

DEFINE ACTION $oppty_forecast_monthly_script() AS

    $period_start = CurrentUtcDateTime();
    $period_end = DateTime::MakeDatetime(
        DateTime::ShiftYears(DateTime::StartOfMonth(CurrentUtcDateTime()), 1)
    );

    $all_dates = Cast(ListFromRange(
        Unwrap(Cast($period_start as DateTime)),
        Unwrap(Cast($period_end as DateTime)),
        Unwrap(DateTime::IntervalFromDays(1))
    ) AS List<Date?>);

    $months = (
        SELECT DISTINCT
            DateTime::MakeDate(
            DateTime::StartOfMonth(dates)
            ) as months
        FROM(
            SELECT
                $all_dates as dates
        )
        FLATTEN BY dates
    );

    $oppty_raw = (
        SELECT
            acc_date_entered,
            acc_date_modified,
            acc_fullname,
            acc_id as crm_id,
            acc_industry,
            acc_name,
            ba_id,
            country_directory,
            dim_name,
            (dim_name like '>%') as is_product_dim_mapped,
            opp_currency,
            opp_date_close_by,
            opp_date_ent,
            opp_date_modified,
            DateTime::MakeDate(DateTime::StartOfMonth(
                COALESCE($str_to_date(opp_expected_close_date), 
                    $str_to_date(opp_expected_close_month)
                )
            )) as opp_expected_close_month,
            DateTime::MakeDate(COALESCE($str_to_date(opp_expected_close_date), 
                DateTime::StartOfMonth($str_to_date(opp_expected_close_month))
            ))  as opp_expected_close_date,
            DateTime::MakeDate(DateTime::ShiftMonths(
                DateTime::StartOfMonth(
                    COALESCE($str_to_date(opp_expected_close_date), 
                        $str_to_date(opp_expected_close_month)
                    )
                ), 1
            )) as month_after_opp_expected_close_month,
            opp_id,
            COALESCE(opp_likely, 0) as opp_likely,
            opp_name,
            opp_non_recurring,
            opp_probability,
            COALESCE(opp_probability_percentages, 0) as opp_probability_percentages,
            opp_sales_stage,
            opp_user_name,
            partner_name,
            partner_value,
            revenue_originated,
            segment
        FROM $oppty_path
    );

    $oppty_with_prod_dim = (
        SELECT
            acc_date_entered,
            acc_date_modified,
            acc_fullname,
            crm_id,
            acc_industry,
            acc_name,
            AGGREGATE_LIST(ba_id) as ba_id_array,
            country_directory,
            dim_name,
            is_product_dim_mapped,
            opp_currency,
            opp_date_close_by,
            opp_date_ent,
            opp_date_modified,
            opp_expected_close_month,
            opp_expected_close_date,
            month_after_opp_expected_close_month,
            opp_id,
            opp_likely,
            opp_name,
            opp_non_recurring,
            opp_probability,
            opp_probability_percentages,
            opp_sales_stage,
            opp_user_name,
            partner_name,
            partner_value,
            revenue_originated,
            segment
        FROM $oppty_raw
        WHERE
            is_product_dim_mapped = True
            AND dim_name != '>Unknown'
        GROUP BY
            acc_date_entered,
            acc_date_modified,
            acc_fullname,
            crm_id,
            acc_industry,
            acc_name,
            country_directory,
            dim_name,
            is_product_dim_mapped,
            opp_currency,
            opp_date_close_by,
            opp_date_ent,
            opp_date_modified,
            opp_expected_close_month,
            opp_expected_close_date,
            month_after_opp_expected_close_month,
            opp_id,
            opp_likely,
            opp_name,
            opp_non_recurring,
            opp_probability,
            opp_probability_percentages,
            opp_sales_stage,
            opp_user_name,
            partner_name,
            partner_value,
            revenue_originated,
            segment
    );

    $oppty_without_prod_dim = (
        SELECT
            acc_date_entered,
            acc_date_modified,
            acc_fullname,
            crm_id,
            acc_industry,
            acc_name,
            AGGREGATE_LIST(ba_id) as ba_id_array,
            country_directory,
            false as is_product_dim_mapped,
            opp_currency,
            opp_date_close_by,
            opp_date_ent,
            opp_date_modified,
            opp_expected_close_month,
            opp_expected_close_date,
            month_after_opp_expected_close_month,
            opp_id,
            opp_likely,
            opp_name,
            opp_non_recurring,
            opp_probability,
            opp_probability_percentages,
            opp_sales_stage,
            opp_user_name,
            partner_name,
            partner_value,
            revenue_originated,
            segment
        FROM $oppty_raw
        WHERE
            opp_id not in (SELECT DISTINCT opp_id FROM $oppty_with_prod_dim)
        GROUP BY
            acc_date_entered,
            acc_date_modified,
            acc_fullname,
            crm_id,
            acc_industry,
            acc_name,
            country_directory,
            opp_currency,
            opp_date_close_by,
            opp_date_ent,
            opp_date_modified,
            opp_expected_close_month,
            opp_expected_close_date,
            month_after_opp_expected_close_month,
            opp_id,
            opp_likely,
            opp_name,
            opp_non_recurring,
            opp_probability,
            opp_probability_percentages,
            opp_sales_stage,
            opp_user_name,
            partner_name,
            partner_value,
            revenue_originated,
            segment
    );

    $new_revenue_raw = (
        SELECT
            IF(segment in ('Mass_indiv', 'Mass_switzerland'), 'Mass',
                IF(segment in ('SMB', 'Medium_Mass_C'), 'Medium', segment)
            ) as segment,
            service_group,
            service_group_dimension,
            new_revenue
        FROM $new_revenue_2022_path
        WHERE year = 2022
    );

    $new_revenue = (
        SELECT
            segment,
            service_group,
            service_group_dimension,
            sum(new_revenue) as new_revenue
        FROM $new_revenue_raw
        GROUP BY
            segment,
            service_group,
            service_group_dimension
    );


    $new_revenue_share_per_segment = (
        SELECT
            segment,
            service_group,
            service_group_dimension,
            new_revenue / (sum(new_revenue) over (partition by segment)) as new_revenue_share
        FROM $new_revenue
    );

    $oppty_with_prod_dim_joined = (
        SELECT
            oppty_with_prod_dim.acc_date_entered as acc_date_entered,
            oppty_with_prod_dim.acc_date_modified as acc_date_modified,
            oppty_with_prod_dim.acc_fullname as acc_fullname,
            oppty_with_prod_dim.crm_id as crm_id,
            oppty_with_prod_dim.acc_industry as acc_industry,
            oppty_with_prod_dim.acc_name as acc_name,
            oppty_with_prod_dim.ba_id_array as ba_id_array,
            oppty_with_prod_dim.country_directory as country_directory,
            oppty_with_prod_dim.is_product_dim_mapped as is_product_dim_mapped,
            oppty_with_prod_dim.opp_currency as opp_currency,
            oppty_with_prod_dim.opp_date_close_by as opp_date_close_by,
            oppty_with_prod_dim.opp_date_ent as opp_date_ent,
            oppty_with_prod_dim.opp_date_modified as opp_date_modified,
            oppty_with_prod_dim.opp_expected_close_month as opp_expected_close_month,
            oppty_with_prod_dim.opp_expected_close_date as opp_expected_close_date,
            oppty_with_prod_dim.month_after_opp_expected_close_month as month_after_opp_expected_close_month,
            oppty_with_prod_dim.opp_id as opp_id,
            oppty_with_prod_dim.opp_likely as opp_likely,
            oppty_with_prod_dim.opp_name as opp_name,
            oppty_with_prod_dim.opp_non_recurring as opp_non_recurring,
            oppty_with_prod_dim.opp_probability as opp_probability,
            oppty_with_prod_dim.opp_probability_percentages as opp_probability_percentages,
            oppty_with_prod_dim.opp_sales_stage as opp_sales_stage,
            oppty_with_prod_dim.opp_user_name as opp_user_name,
            oppty_with_prod_dim.partner_name as partner_name,
            oppty_with_prod_dim.partner_value as partner_value,
            oppty_with_prod_dim.revenue_originated as revenue_originated,
            oppty_with_prod_dim.segment as segment,
            new_revenue.service_group as service_group,
            (new_revenue.new_revenue_share / 
                sum(new_revenue.new_revenue_share) over (partition by oppty_with_prod_dim.opp_id)) as opp_share
        FROM $oppty_with_prod_dim as oppty_with_prod_dim
        LEFT JOIN $new_revenue_share_per_segment as new_revenue
        ON oppty_with_prod_dim.segment = new_revenue.segment
            AND oppty_with_prod_dim.dim_name = new_revenue.service_group_dimension
    );

    $oppty_without_prod_dim_joined = (
        SELECT
            oppty_with_prod_dim.acc_date_entered as acc_date_entered,
            oppty_with_prod_dim.acc_date_modified as acc_date_modified,
            oppty_with_prod_dim.acc_fullname as acc_fullname,
            oppty_with_prod_dim.crm_id as crm_id,
            oppty_with_prod_dim.acc_industry as acc_industry,
            oppty_with_prod_dim.acc_name as acc_name,
            oppty_with_prod_dim.ba_id_array as ba_id_array,
            oppty_with_prod_dim.country_directory as country_directory,
            oppty_with_prod_dim.is_product_dim_mapped as is_product_dim_mapped,
            oppty_with_prod_dim.opp_currency as opp_currency,
            oppty_with_prod_dim.opp_date_close_by as opp_date_close_by,
            oppty_with_prod_dim.opp_date_ent as opp_date_ent,
            oppty_with_prod_dim.opp_date_modified as opp_date_modified,
            oppty_with_prod_dim.opp_expected_close_month as opp_expected_close_month,
            oppty_with_prod_dim.opp_expected_close_date as opp_expected_close_date,
            oppty_with_prod_dim.month_after_opp_expected_close_month as month_after_opp_expected_close_month,
            oppty_with_prod_dim.opp_id as opp_id,
            oppty_with_prod_dim.opp_likely as opp_likely,
            oppty_with_prod_dim.opp_name as opp_name,
            oppty_with_prod_dim.opp_non_recurring as opp_non_recurring,
            oppty_with_prod_dim.opp_probability as opp_probability,
            oppty_with_prod_dim.opp_probability_percentages as opp_probability_percentages,
            oppty_with_prod_dim.opp_sales_stage as opp_sales_stage,
            oppty_with_prod_dim.opp_user_name as opp_user_name,
            oppty_with_prod_dim.partner_name as partner_name,
            oppty_with_prod_dim.partner_value as partner_value,
            oppty_with_prod_dim.revenue_originated as revenue_originated,
            oppty_with_prod_dim.segment as segment,
            new_revenue.service_group as service_group,
            (new_revenue.new_revenue_share / 
                sum(new_revenue.new_revenue_share) over (partition by oppty_with_prod_dim.opp_id)) as opp_share
        FROM $oppty_without_prod_dim as oppty_with_prod_dim
        LEFT JOIN $new_revenue_share_per_segment as new_revenue
        ON oppty_with_prod_dim.segment = new_revenue.segment
    );

    $oppty_joined = (
        SELECT *
        FROM $oppty_with_prod_dim_joined
        UNION ALL
        SELECT *
        FROM $oppty_without_prod_dim_joined
    );

    $oppty_forecast = (
        SELECT
            oppty_joined.acc_date_entered                                       as acc_date_entered,
            oppty_joined.acc_date_modified                                      as acc_date_modified,
            oppty_joined.acc_fullname                                           as acc_fullname,
            oppty_joined.crm_id                                                 as crm_id,
            oppty_joined.acc_industry                                           as acc_industry,
            oppty_joined.acc_name                                               as acc_name,
            oppty_joined.ba_id_array                                            as ba_id_array,
            oppty_joined.country_directory                                      as country_directory,
            oppty_joined.is_product_dim_mapped                                  as is_product_dim_mapped,
            oppty_joined.opp_currency                                           as opp_currency,
            oppty_joined.opp_date_close_by                                      as opp_date_close_by,
            oppty_joined.opp_date_ent                                           as opp_date_ent,
            oppty_joined.opp_date_modified                                      as opp_date_modified,
            oppty_joined.opp_expected_close_month                               as opp_expected_close_month,
            (oppty_joined.opp_expected_close_month >= 
                DateTime::MakeDate(DateTime::StartOfMonth(CurrentUtcDateTime()))
            )                                                                   as opp_curently_active,
            oppty_joined.opp_expected_close_date                                as opp_expected_close_date,
            oppty_joined.month_after_opp_expected_close_month                   as month_after_opp_expected_close_month,
            CAST(DateTime::ToDays(oppty_joined.month_after_opp_expected_close_month - 
                oppty_joined.opp_expected_close_date) as double) /
                CAST(DateTime::ToDays(oppty_joined.month_after_opp_expected_close_month - 
                    oppty_joined.opp_expected_close_month) as double)           as share_of_first_month,
            oppty_joined.opp_id                                                 as opp_id,
            oppty_joined.opp_likely                                             as opp_likely,
            oppty_joined.opp_name                                               as opp_name,
            oppty_joined.opp_non_recurring                                      as opp_non_recurring,
            oppty_joined.opp_probability                                        as opp_probability,
            oppty_joined.opp_probability_percentages                            as opp_probability_percentages,
            oppty_joined.opp_sales_stage                                        as opp_sales_stage,
            oppty_joined.opp_user_name                                          as opp_user_name,
            oppty_joined.partner_name                                           as partner_name,
            oppty_joined.partner_value                                          as partner_value,
            oppty_joined.revenue_originated                                     as revenue_originated,
            oppty_joined.segment                                                as segment,
            oppty_joined.service_group                                          as service_group,
            oppty_joined.opp_share                                              as opp_share,
            (
                if(
                    oppty_joined.opp_non_recurring != true,
                    oppty_joined.opp_likely,
                    if(
                        months.months = oppty_joined.opp_expected_close_month,
                        oppty_joined.opp_likely,
                        0
                    ) 
                ) * 
                oppty_joined.opp_probability_percentages / 100 * 
                oppty_joined.opp_share *
                if(
                    months.months = oppty_joined.opp_expected_close_month, 
                    CAST(DateTime::ToDays(oppty_joined.month_after_opp_expected_close_month - 
                        oppty_joined.opp_expected_close_date) as double) /
                        CAST(DateTime::ToDays(oppty_joined.month_after_opp_expected_close_month - 
                            oppty_joined.opp_expected_close_month) as double),
                    1
                )
            )                                                                   as opp_forecast,
            months.months                                                       as months
        FROM $oppty_joined as oppty_joined
        CROSS JOIN $months as months
        WHERE months.months >= oppty_joined.opp_expected_close_month
    );

    INSERT INTO $final_table_path WITH TRUNCATE 
        SELECT *
        FROM $oppty_forecast
        ORDER BY
            months,
            opp_id,
            is_product_dim_mapped,
            opp_probability

END DEFINE;

EXPORT $oppty_forecast_monthly_script;

use hahn;

$parse = DateTime::Parse('%Y-%m-%d %H:%M:%S');
$parse_d = DateTime::Parse('%Y-%m-%d');
$format = DateTime::Format('%Y-%m-%d');
$today = $format(DateTime::FromMicroseconds(YQL::Now()));

$sales_cycle_data = (
SELECT
a.billing_account_id as billing_account_id,
ba_created_at_crm,
ba_created_at_,
COALESCE(ba_created_at_crm, ba_created_at_) as ba_created_at,
board_segment,
w_paid,
w_10k,
w_300k
FROM (
SELECT
billing_account_id,
min(w) as w_paid,
min(if(paid_mrr>10000, w, $parse_d('2099-12-31'))) as w_10k,
min(if(paid_mrr>300000, w, $parse_d('2099-12-31'))) as w_300k
FROM (
SELECT
    billing_account_id,
    w,
    sum(real_consumption_vat)*52/12 as paid_mrr
FROM (
    SELECT
        billing_account_id,
        DateTime::StartOfWeek($parse(event_time)) as w,
        real_consumption_vat
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE event = 'day_use'
    )
GROUP BY
    billing_account_id, w
)
GROUP BY
    billing_account_id

) as a LEFT JOIN (
SELECT
    ba_id,
    DateTime::FromSeconds(CAST(min(date_entered) AS Uint32)) as ba_created_at_crm
FROM RANGE ('//home/cloud_analytics/dwh/raw/crm/accounts')
GROUP BY ba_id
) as b
ON a.billing_account_id = b.ba_id
LEFT JOIN (
SELECT
        billing_account_id,
        $parse(MAX_BY(first_ba_created_datetime, event_time)) as ba_created_at_,
        MAX_BY(board_segment, event_time) as board_segment,
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE 1=1
    AND event = 'day_use'
    GROUP BY
        billing_account_id
) as c
ON a.billing_account_id = c.billing_account_id
);

$result = (
SELECT
    `date`,
    costs_type,
    if(costs_type in ('Marketing Perfomance','Marketing Events'),
       'Marketing',
       if(costs_type = 'Revenue (w/o VAT)',
          'Revenue',
          if(costs_type in ('Enterprise Sales', 'CSM Sales', 'BizDev ML', 'BizDev (non-ML)', 'Telesales'),
             'Sales',
             if (costs_type in ('Hardware (Trial)','Hardware (Paid)', 'Windows (Trial)', 'Windows (Paid)'),
                 'Hardware',
                 if(costs_type in ('Support', 'Professional Services'),
                 'Support', 'Other'))))) as costs_group,
    a.billing_account_id as billing_account_id,
    costs,
    coalesce(account_name, 'unknown') as account_name,
    coalesce(ba_person_type, 'unknown') as ba_person_type,
    coalesce(ba_usage_status, 'unknown') as ba_usage_status,
    coalesce(ba_type, 'unknown') as ba_type,
    coalesce(sales_name, 'unknown') as sales_name,

    coalesce(segment, 'Mass') as segment,
    coalesce(is_fraud, 0) as is_fraud,
    coalesce(d.board_segment, 'mass') as board_segment,
    coalesce(ba_name, 'unknown') as ba_name,
    coalesce(block_reason, 'unknown') as block_reason,

    coalesce(architect, 'unknown') as architect,
    coalesce(ba_created_date_, '2099-12-31') as ba_created_date,
    coalesce(m_cohort, 'unknown') as m_cohort,
    coalesce($format(ba_paid_date_), $format(Date('2099-12-31'))) as ba_paid_date,
    if(coalesce(d.board_segment, 'mass') = 'mass',
        if(DateTime::MakeDate($parse_d(`date`))<coalesce(ba_paid_date_, Date('2099-12-31')), 'acquisition', 'retention'),
        if(coalesce(d.board_segment, 'mass') = 'medium',
            if(DateTime::ToDays(DateTime::MakeDate($parse_d(`date`)) - ba_created_at)/7.0 < 20,
                if(DateTime::MakeDate($parse_d(`date`))<coalesce(w_10k, Date('2099-12-31')), 'acquisition', 'retention'),
                'retention'
                ),
            if(coalesce(d.board_segment, 'mass') = 'large',
                if(DateTime::ToDays(DateTime::MakeDate($parse_d(`date`)) - ba_created_at) < 365,
                    if(DateTime::MakeDate($parse_d(`date`))<coalesce(w_300k,  Date('2099-12-31')), 'acquisition', 'retention'),
                    'retention'
                    ),
                'acquisition'
                )
            ),

        ) as sales_cycle_step
FROM (
SELECT
    `date`,
    costs_type,
    costs,
    billing_account_id
FROM (
SELECT *
FROM `//home/cloud_analytics/unit_economy/architects_costs/architects_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/support_costs/support_costs`


UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/marketing_events_costs/marketing_events_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/marketing_perfomance_costs/marketing_perfomace_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/telesales_costs/telesales_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/ent_sales_costs/ent_sales_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/csm_sales_costs/csm_sales_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/bizdev_ml_costs/bizdev_ml_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/bizdev_non_ml_costs/bizdev_non_ml_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/hardware_costs/hardware_costs`

UNION ALL

SELECT *
FROM `//home/cloud_analytics/unit_economy/ps_costs/ps_costs`

UNION ALL

SELECT
   `date`,
    costs_type,
    sum(real_consumption_vat) as costs,
    billing_account_id
FROM    (SELECT
    $format($parse(event_time)) as `date`,
    'Revenue (w/o VAT)' as costs_type,
    real_consumption_vat,
    billing_account_id
FROM
    `//home/cloud_analytics/cubes/acquisition_cube/cube`
WHERE event = 'day_use')
GROUP BY
    `date`, costs_type, billing_account_id
)

UNION ALL

SELECT
   `date`,
    costs_type,
    sum(real_consumption_vat) as costs,
    billing_account_id
FROM    (SELECT
    $format($parse(event_time)) as `date`,
    'Windows (Paid)' as costs_type,
    real_consumption_vat,
    billing_account_id
FROM
    `//home/cloud_analytics/cubes/acquisition_cube/cube`
WHERE event = 'day_use' and service_name = 'marketplace' and subservice_name = 'windows')
GROUP BY
    `date`, costs_type, billing_account_id



UNION ALL

SELECT
   `date`,
    costs_type,
    sum(trial_consumption_vat) as costs,
    billing_account_id
FROM    (SELECT
    $format($parse(event_time)) as `date`,
    'Windows (Trial)' as costs_type,
    trial_consumption_vat,
    billing_account_id
FROM
    `//home/cloud_analytics/cubes/acquisition_cube/cube`
WHERE event = 'day_use' and service_name = 'marketplace' and subservice_name = 'windows')
GROUP BY
    `date`, costs_type, billing_account_id
) as a
LEFT JOIN (
    SELECT
        billing_account_id,
        $format($parse(MAX_BY(first_ba_created_datetime, event_time))) as ba_created_date_,
        $format(DateTime::StartOfMonth($parse(MAX_BY(first_ba_created_datetime, event_time)))) as m_cohort,
        $parse(MAX_BY(first_first_paid_consumption_datetime, event_time)) as ba_paid_date_,
        MAX_BY(account_name, event_time) as account_name,
        MAX_BY(ba_person_type, event_time) as ba_person_type,
        MAX_BY(ba_usage_status, event_time) as ba_usage_status,
        MAX_BY(ba_type, event_time) as ba_type,
        MAX_BY(sales_name, event_time) as sales_name,
        MAX_BY(segment, event_time) as segment,
        MAX_BY(is_fraud, 0) as is_fraud,
        MAX_BY(board_segment, event_time) as board_segment,
        MAX_BY(ba_name, event_time) as ba_name,
        MAX_BY(block_reason, event_time) as block_reason,
        MAX_BY(architect, event_time) as architect
    FROM
        `//home/cloud_analytics/cubes/acquisition_cube/cube`
    WHERE 1=1
    AND event = 'day_use'
    GROUP BY
        billing_account_id
) as c
ON a.billing_account_id = c.billing_account_id
LEFT JOIN $sales_cycle_data as d
ON a.billing_account_id = d.billing_account_id
)
;


INSERT INTO `//home/cloud_analytics/unit_economy/total_costs/total_costs` WITH TRUNCATE
SELECT *
FROM $result;

$result_history_path = '//home/cloud_analytics/unit_economy/total_costs/history/' || $today;

INSERT INTO $result_history_path WITH TRUNCATE
SELECT *
FROM $result;
USE hahn;

$contract_limit = 1000000;  -- consider contracts with sum more than $contract_limit RUB
$proba_threshold = 80;  -- consider contracts with probability more than $proba_threshold %

$res_table = '//home/cloud_analytics/ml/capacity_planning/opportunities/contracts';
$opportunities = '//home/cloud-dwh/data/prod/ods/crm/crm_opportunities';
$opportunity_resources = '//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resources';
$opportunity_resource_lines = '//home/cloud-dwh/data/prod/ods/crm/crm_opportunity_resource_lines';

$last_month_date = ($dt) -> (DateTime::MakeDate(DateTime::StartOfMonth(DateTime::ShiftMonths($dt, 1))) - DateTime::IntervalFromDays(1));
$dt_format = DateTime::Format('%Y-%m-%d');

$oppty_lines =
    SELECT
        opportunity_resource_lines.crm_opportunity_id AS opportunity_id,
        opportunity_resource_lines.crm_opportunity_resource_id AS opportunity_resource_id,
        opportunity_resources.crm_opportunity_resource_name AS name,
        opportunity_resource_lines.quantity AS quantity,
        opportunity_resources.units AS units
    FROM $opportunity_resource_lines AS opportunity_resource_lines
    JOIN $opportunity_resources AS opportunity_resources
        ON opportunity_resource_lines.crm_opportunity_resource_id = opportunity_resources.crm_opportunity_resource_id
;

$most_possible_large_contracts =
    SELECT
        oppty.crm_opportunity_id AS id,
        oppty.amount AS amount,
        $dt_format($last_month_date(Coalesce($last_month_date(oppty.date_closed_month), oppty.date_closed))) AS expected_close_month,
        oppty_lines.name AS name,
        oppty_lines.quantity AS quantity,
        oppty_lines.units AS units
    FROM $opportunities AS oppty
    LEFT JOIN $oppty_lines AS oppty_lines ON oppty.crm_opportunity_id = oppty_lines.opportunity_id
    WHERE oppty.deleted = False
        AND oppty.lost_reason IS Null
        AND oppty.amount >= $contract_limit
        AND oppty.probability >= $proba_threshold
        AND Coalesce($last_month_date(oppty.date_closed_month), oppty.date_closed) >= CurrentUtcDate()
;

INSERT INTO $res_table WITH TRUNCATE
    SELECT rep_date, name, expected_close_month, quantity, units
    FROM (
        SELECT rep_date, name, expected_close_month, quantity, units
        FROM $res_table
        WHERE rep_date < $dt_format(CurrentUtcDate())
        UNION ALL
        SELECT
            $dt_format(CurrentUtcDate()) AS rep_date,
            name,
            expected_close_month,
            Coalesce(Sum(quantity), 0) AS quantity,
            Some(units) AS units
        FROM $most_possible_large_contracts
        WHERE name IS NOT Null AND quantity > 100
        GROUP BY name, expected_close_month
    ) AS t
    ORDER BY rep_date, name
;

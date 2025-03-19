CREATE VIEW cloud_analytics_testing.calls_cohorts AS 
    SELECT
        lead_id,
        ba_payment_cycle_type,
        ba_state,
        ba_person_type,
        ba_usage_status,
        call_status, call_tag,
        channel,
        lead_source,
        lead_source_description,
        sales_name,
        segment,
        call_date,
        date,
        first_paid_consumption
    FROM(
        SELECT
            lead_id,
            ba_payment_cycle_type,
            ba_state,
            ba_person_type,
            ba_usage_status,
            call_status, call_tag,
            channel,
            lead_source,
            lead_source_description,
            sales_name,
            segment,
            groupArray(date) as dates,
            arrayMap(x -> x > 0, arrayCumSum(groupArray(first_paid_consumption))) as first_paid_consumptions,
            dates[1] as call_date
        FROM(
            SELECT
                t0.*,
                t1.first_paid_consumption
            FROM(
            SELECT
                lead_id,
                ba_payment_cycle_type,
                ba_state,
                ba_person_type,
                ba_usage_status,
                call_status, call_tag,
                channel,
                lead_source,
                lead_source_description,
                sales_name,
                segment,
                date
            FROM(
                SELECT
                    lead_id,
                    ba_payment_cycle_type,
                    ba_state,
                    ba_person_type,
                    ba_usage_status,
                    call_status, call_tag,
                    channel,
                    lead_source,
                    lead_source_description,
                    sales_name,
                    segment,
                    arrayMap(x -> addDays(toDate(event_time), x) ,range(toUInt32(toDate(now()) - toDate(event_time))) ) as date_range
                FROM
                    (
                    SELECT
                      *
                    FROM
                      cloud_analytics_testing.crm_lead_cube_test
                    ORDER BY
                      event_time
                  )
                 WHERE
                  event = 'call'
            )
            ARRAY JOIN date_range AS date
            ) as t0
            ANY LEFT JOIN (
                SELECT
                    lead_id,
                    toDate(event_time) as date,
                    1 as first_paid_consumption
                FROM
                    cloud_analytics_testing.crm_lead_cube_test
                WHERE
                    lead_state = 'Paid Consumption'
            ) as t1
            ON t1.lead_id = t0.lead_id
        )
        GROUP BY
            lead_id,
            ba_payment_cycle_type,
            ba_state,
            ba_person_type,
            ba_usage_status,
            call_status, call_tag,
            channel,
            lead_source,
            lead_source_description,
            sales_name,
            segment
    )
    ARRAY JOIN dates AS date, first_paid_consumptions AS first_paid_consumption
USE hahn;
PRAGMA DisableSimpleColumns;
PRAGMA yson.DisableStrict;
IMPORT time SYMBOLS $date_time;



DEFINE SUBQUERY $consumption_for_schema($billing_metrics_table, $schema) AS

    $acq="//home/cloud_analytics/cubes/acquisition_cube/cube";
    $cloud_created = (SELECT DISTINCT
                            billing_account_id,
                            account_name,
                            cloud_id
                    FROM $acq
                    WHERE event='day_use' AND billing_account_id IS NOT NULL
                        AND ba_usage_status!='service');

    

    $billing_metrics = (SELECT  folder_id,
                                schema, 
                                usage,
                                tags,
                                resource_id,
                                cloud_id,
                                CAST(Yson::LookupUint64(usage, 'start') as Datetime) AS t_start
                        FROM RANGE($billing_metrics_table)
                        WHERE  Yson::LookupString(usage, 'quantity') IS NOT NULL
                            AND ((schema = $schema) OR ($schema='all')));
    
    $trial_start = (SELECT  cloud_id,
                            MIN(t_start) AS cons_start_time
                    FROM $billing_metrics
                    GROUP BY cloud_id);
                        

    SELECT acq.billing_account_id AS billing_account_id, 
        acq.account_name AS account_name, 
        bm.*
    FROM $billing_metrics AS bm
    INNER JOIN $cloud_created AS acq ON bm.cloud_id = acq.cloud_id
    INNER JOIN $trial_start AS ts ON acq.cloud_id = ts.cloud_id
    WHERE (bm.t_start >= ts.cons_start_time) AND (bm.t_start <= (ts.cons_start_time + INTERVAL('PT1H')));
END DEFINE;

EXPORT $consumption_for_schema;
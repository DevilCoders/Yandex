USE hahn;
PRAGMA Library("time.sql");

IMPORT time SYMBOLS $date_time;


$features_table = "//home/cloud_analytics/tmp/antifraud/same_ip";
$result_table = "//home/cloud_analytics/tmp/antifraud/same_phone";


$acq="//home/cloud_analytics/cubes/acquisition_cube/cube";
$first_day_cons = (SELECT 
                        billing_account_id,
                        MIN($date_time(event_time)) as trial_start, 
                FROM $acq
                WHERE event='day_use' AND billing_account_id IS NOT NULL
                    AND ba_usage_status!='service'
                GROUP BY  billing_account_id
                    );

$first_phone = (SELECT  distinct fc.billing_account_id as billing_account_id    , phone
                FROM $acq as a
                      inner join $first_day_cons   as fc 
                        on a.billing_account_id = fc.billing_account_id and $date_time(a.event_time) = fc.trial_start
                WHERE event='day_use' AND fc.billing_account_id IS NOT NULL
                    AND ba_usage_status!='service'
                    );
        


$pairs = (
select ft1.billing_account_id as billing_account_id, ft2.billing_account_id as other_ba 
from $first_phone as ft1 
inner join $first_phone as ft2 on ft1.phone = ft2.phone
);




$same_phone = (select billing_account_id, count(other_ba) as same_phone
            from $pairs
            group by billing_account_id
            having count(other_ba)>0);

INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $same_phone as dp on ft.billing_account_id = dp.billing_account_id
left join $first_phone as fp on fp.billing_account_id = ft.billing_account_id



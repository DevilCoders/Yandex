USE hahn;
PRAGMA Library('time.sql');
IMPORT time SYMBOLS $date_time;


$all_1_after_trial= "//home/cloud_analytics/tmp/antifraud/all_1h_after_trial";
$features_table = "//home/cloud_analytics/tmp/antifraud/compute_serv";
$result_table = "//home/cloud_analytics/tmp/antifraud/days_after_ba";
$acq="//home/cloud_analytics/cubes/acquisition_cube/cube";


$ba_created = (SELECT 
                        billing_account_id,
                        $date_time(event_time) as ba_time,
                FROM $acq
                WHERE event='ba_created' AND billing_account_id IS NOT NULL
                    AND ba_usage_status!='service');
                    
                    
$trial_start = (select billing_account_id, min(`bm.t_start`) as trial_start
                from $all_1_after_trial
                group by billing_account_id);
                
                
$days_diff = (select ts.billing_account_id as billing_account_id, 
                     DateTime::ToDays(trial_start - ba_time) as days_after_ba
              from $trial_start as ts
              left join $ba_created as ba on ba.billing_account_id = ts.billing_account_id);
              


                    


INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $days_diff as su on ft.billing_account_id = su.billing_account_id


USE hahn;
PRAGMA Library('time.sql');
IMPORT time SYMBOLS $date_time;


$all_1_after_trial= "//home/cloud_analytics/tmp/antifraud/all_1h_after_trial";
$features_table = "//home/cloud_analytics/tmp/antifraud/days_after_ba";
$result_table = "//home/cloud_analytics/tmp/antifraud/days_after_puid";
$acq="//home/cloud_analytics/cubes/acquisition_cube/cube";
$accounts_state = "//home/antifraud/export/accounts/accounts_state";


$ba_created = (SELECT   CAST(puid as Uint64) as passport_id,
                        billing_account_id,
                        $date_time(event_time) as ba_time,
                        cast(channel = 'Unknown' as Uint32) as unknown_channel,
                        cast(channel = 'Direct' as Uint32) as direct_channel
                FROM $acq
                WHERE event='ba_created' AND billing_account_id IS NOT NULL
                    AND ba_usage_status!='service');
                    
                    
$trial_start = (select billing_account_id, min(`bm.t_start`) as trial_start
                from $all_1_after_trial
                group by billing_account_id);
                

$days_after_puid = (
    select ba.*,
           DateTime::ToDays(ba_time - $date_time(reg_date)) as ba_puid_diff,
           DateTime::ToDays(trial_start - $date_time(reg_date)) as trial_puid_diff,
           cast((ba.passport_id < 10000000000) as Uint32) as puid_threshold,
           trial_start
    from $ba_created as ba
    inner join $accounts_state as acc on ba.passport_id = acc.uid
    inner join $trial_start as ts  on ba.billing_account_id = ts.billing_account_id);
    

INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $days_after_puid as dp on ft.billing_account_id = dp.billing_account_id

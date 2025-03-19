
$bad_accounts_history = "//home/cloud_analytics/tmp/antifraud/bad_accounts_history";
$result_table = "//home/cloud_analytics/tmp/antifraud/passport_rules";
$features_table = "//home/cloud_analytics/tmp/antifraud/same_phone";

$features_time_diff = (SELECT ft.*, 
                              bad.*, 
                             Math::Abs(DateTime::ToSeconds(bad.time) 
                             - DateTime::ToSeconds(ft.trial_start)) / 60 as time_diff 
                        WITHOUT bad.passport_uid
              FROM $features_table as ft
              LEFT JOIN $bad_accounts_history as bad on ft.passport_id = bad.passport_uid);


$min_time_diff = (SELECT passport_id as passport_id, 
                          MIN(time_diff) as min_time_diff
                  FROM $features_time_diff
                  GROUP BY passport_id);

INSERT INTO $result_table WITH TRUNCATE  
select * 
from $features_time_diff  as ft
left join $min_time_diff  as min_diff 
    on ft.passport_id = min_diff.passport_id
where ft.time_diff = min_diff.min_time_diff or (time_diff is null)




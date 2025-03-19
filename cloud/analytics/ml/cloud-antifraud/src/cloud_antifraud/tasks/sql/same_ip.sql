USE hahn;
IMPORT time SYMBOLS $date_time;


$features_table = "//home/cloud_analytics/tmp/antifraud/passport_reg";
$result_table = "//home/cloud_analytics/tmp/antifraud/same_ip";



$pairs = (
select ft1.billing_account_id as billing_account_id, ft2.billing_account_id as other_ba 
from $features_table as ft1 
inner join $features_table as ft2 on ft1.registration_ip = ft2.registration_ip
where (ft1.billing_account_id != '') and (ft2.billing_account_id!='')
and (ft1.billing_account_id is not null) and (ft2.billing_account_id is not null)
and Math::Abs(DateTime::ToDays(ft1.ba_time - ft2.ba_time)) < 10
and ft2.ba_time <= ft1.ba_time
);




$same_ip = (select billing_account_id, count(other_ba) as same_ip
            from $pairs
            group by billing_account_id
            having count(other_ba)>0);

INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $same_ip as dp on ft.billing_account_id = dp.billing_account_id

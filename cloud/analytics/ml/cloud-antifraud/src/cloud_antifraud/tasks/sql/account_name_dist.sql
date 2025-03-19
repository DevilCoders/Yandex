USE hahn;
PRAGMA Library('time.sql');
IMPORT time SYMBOLS $date_time;


$features_table = "//home/cloud_analytics/tmp/antifraud/days_after_puid";
$result_table = "//home/cloud_analytics/tmp/antifraud/account_name_dist";



$script2 = @@
def hamming(s1, s2):

    if s1 is None or s2 is None:
        return 100500
    
    ans = abs(len(s1) - len(s2))
    for i in range(min(len(s1), len(s2))):
        ans += int(s1[i] != s2[i])
    return ans

@@; 

$hamming = Python2::hamming( 
    ParseType("(String?, String?)->Int64?"),   
    $script2
);


$pairs_dist = (select ft1.billing_account_id as billing_account_id, 
                     $hamming(ft1.account_name, ft2.account_name) as account_name_dist
               from $features_table as ft1 
               cross join $features_table as ft2);

$cluster_sizes = (
                SELECT

                    `billing_account_id`,
                    count(account_name_dist) as n_sim_accounts,
                FROM $pairs_dist
                WHERE account_name_dist <= 3
                GROUP BY billing_account_id
                );


INSERT INTO $result_table WITH TRUNCATE  
select *
from $features_table as ft
left join $cluster_sizes as dp on ft.billing_account_id = dp.billing_account_id


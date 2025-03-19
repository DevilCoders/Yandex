USE hahn;
--PRAGMA yt.InferSchema;

$script_yesterday = @@
from datetime import date, timedelta

def get_yesterday():
    return (date.today() - timedelta(days=1)).strftime('%Y-%m-%d')
    
@@;

$get_yesterday = Python::get_yesterday("()->String", $script_yesterday);
$current_date = $get_yesterday();

$daily_checks_folder = "//home/mdb/juggler-stats/daily_checks/";
$table_name = $daily_checks_folder ||  $current_date;

insert into @logins
SELECT  username FROM hahn.`//home/mdb/staff`;
COMMIT;

DROP TABLE $table_name;
COMMIT;

insert into $table_name
SELECT
    $current_date as dt, checks as checks, status as status, count(pid) as c
from range(`//statbox/juggler-banshee-log`, $current_date, $current_date)
WHERE status in("WARN", "CRIT") and login in (select username from @logins)
and event_type = "message_processed"
group by checks, status
ORDER BY c DESC;
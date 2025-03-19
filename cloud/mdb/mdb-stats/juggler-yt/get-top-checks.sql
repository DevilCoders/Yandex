USE hahn;
PRAGMA yt.InferSchema;

$script_yesterday = @@
from datetime import date, timedelta

def get_yesterday():
    return (date.today() - timedelta(days=1)).strftime('%Y-%m-%d')
    
@@;

$script_timedelta = @@
from datetime import date, timedelta

def get_timedelta():
    return (date.today() - timedelta(days=7)).strftime('%Y-%m-%d')
    
@@;


$get_timedelta = Python::get_timedelta("()->String", $script_timedelta);
$get_yesterday = Python::get_yesterday("()->String", $script_yesterday);
$current_date = $get_yesterday(); 
$last_day = $get_timedelta();


insert into @logins
SELECT  username FROM hahn.`//home/mdb/staff`;
COMMIT;

-- TOP checks for the period
SELECT
    checks, status, count(pid) as c
from range(`//statbox/juggler-banshee-log`, $last_day, $current_date)
WHERE status in("WARN", "CRIT") and login in (select username from @logins)
and event_type = "message_processed"
group by checks, status
ORDER BY c, checks DESC;

-- TOP dutys
/*
SELECT
    login, method, status, count(pid) as c
from range(`//statbox/juggler-banshee-log`, $last_day, $current_date)
WHERE status in("WARN", "CRIT") and login in (select username from @logins)
and event_type = "message_processed"
group by login, method, status 
order by c desc;
*/
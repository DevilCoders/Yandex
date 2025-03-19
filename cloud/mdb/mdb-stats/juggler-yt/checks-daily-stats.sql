USE hahn;
PRAGMA yt.InferSchema;

$dt = Re2::Capture("\d{4}-\d{2}-\d{2}");

$daily_checks = "//home/mdb/juggler-stats/daily_checks";
$daily_check_stat = "//home/mdb/juggler-stats/$daily_check_stat";
$daily_notifications = "//home/mdb/juggler-stats/daily_notifications";

$script_ts = @@

import re

def get_ts(query):
    data = re.search(r'\d{4}-\d{2}-\d{2}', query)
    if data:
        return data.group(0)
    else:
        return "Can't parse"

@@;

$get_ts = Python::get_ts("(String?)->String", $script_ts);

insert into @logins
SELECT  username FROM hahn.`//home/mdb/staff`;
COMMIT;

insert into $daily_check_stat
SELECT
    dt, checks as checks, status as status, method, login, count(pid) as c
from range(`//statbox/juggler-banshee-log`, '2019-03-01', '2019-04-08')
WHERE status in("WARN", "CRIT") and login in (select username from @logins)
and event_type = "message_processed"
group by $get_ts(`timestamp`) as dt, checks, status, method, login
ORDER BY dt DESC;
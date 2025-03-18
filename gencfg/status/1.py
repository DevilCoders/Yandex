import time
from datetime import datetime

a = open('123', 'r').readlines()

dates = {}
prev_commit_ts = 0
prev_status = 1
prev_date = None
unworking = 0
for c in a:
    b = c.strip().split(' ')

    date = b[1]
    dates.setdefault(date, 0)
    status = int(b[3])
    commit_ts = int(time.mktime(datetime.strptime(b[1] + ' ' + b[2], '%Y-%m-%d %H:%M:%S').timetuple()))
    delta = commit_ts - prev_commit_ts

    if date != prev_date and prev_status == 0:
        date_ts = int(time.mktime(datetime.strptime(date, '%Y-%m-%d').timetuple()))
        unworking += date_ts - prev_commit_ts
        dates[prev_date] += unworking
        unworking = 0
        delta = commit_ts - date_ts

    if prev_status == 0:
        unworking += delta
    else:
        unworking = 0

    if status == 1:
        dates[date] += unworking

    prev_commit_ts = commit_ts
    prev_status = status
    prev_date = date

print 'date;percent'
for d in sorted(dates):
    print(d + ';' + str((60 * 60 * 24 - dates[d]) * 100 / 60 / 60 / 24))


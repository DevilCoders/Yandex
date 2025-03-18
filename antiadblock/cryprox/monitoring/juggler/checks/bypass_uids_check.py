#!/usr/bin/python -u
import os
import time


event_text = 'PASSIVE-CHECK:{uids_type};{status};{descr}'
now = int(time.time())
for uids_type in ('ANTIADBLOCK_BYPASS_UIDS_DESKTOP', 'ANTIADBLOCK_BYPASS_UIDS_MOBILE'):
    file_stats = os.stat(os.path.join('/tmp_uids', uids_type))
    seconds_since_last_update = now - file_stats.st_ctime
    print event_text.format(
        status='OK' if seconds_since_last_update < 2 * 24 * 60 * 60 else 'CRIT',
        descr='{} s since update'.format(seconds_since_last_update),
        uids_type=uids_type,
    )

#!/usr/bin/python -u
import datetime
import subprocess


event_text = 'PASSIVE-CHECK:cryprox.oom;{status};{descr}'
iso_format = '%Y-%m-%dT%H:%M:%S'
check_min_time = (datetime.datetime.now() - datetime.timedelta(hours=1)).strftime(iso_format)
oom_kills = len(filter(lambda line: 'cryprox' in line and 'killed' in line and line > check_min_time, subprocess.check_output(['dmesg', '--time-format', 'iso']).split('\n')))
print event_text.format(status='OK' if oom_kills == 0 else 'CRIT', descr='{} oom kills during last hour'.format(oom_kills))

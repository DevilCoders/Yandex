#!/usr/bin/python
import sys
from collections import defaultdict
# python-progressbar
import progressbar
from progressbar import AnimatedMarker, Bar, BouncingBar, Counter, ETA, \
    FileTransferSpeed, FormatLabel, Percentage, \
    ProgressBar, ReverseBar, RotatingMarker, \
    SimpleProgress, Timer, UnknownLength


def tskv_parse(line):
    result = {}
    for x in line[5:].split('\t'):
        spl = x.split('=', 1)
        try:
            key = spl[0]
            value = spl[1]
        except IndexError:
            continue

        result[key] = value

    return result


if len(sys.argv) != 3:
    sys.exit(1)

info = defaultdict(lambda: defaultdict(dict))
karl_log = sys.argv[1]
count_lines = int(sys.argv[2])

widgets = [progressbar.Bar('=', '[', ']'), ' ', Percentage(), ", ", SimpleProgress(), ", ", Timer(), ", ", ETA()]
bar = progressbar.ProgressBar(maxval=count_lines, widgets=widgets)
# bar.start()
i = 0
with open(karl_log, 'r') as f:
    for line in f:
        line = line.rstrip()
        if 'msg=committing key' in line:
            tskv = tskv_parse(line)
            tskv['log_line'] = line
            info[tskv['trace.id']][tskv['couple_id']] = tskv
        elif 'msg=writer: commit' in line:
            tskv = tskv_parse(line)
            info[tskv['trace.id']][tskv['couple_id']]['COMMIT'] = True
        else:
            pass
        # bar.update(i)
        i += 1
# bar.finish()

access = defaultdict(list)
with open('/var/tmp/karl.access.log', 'r') as f:
    for line in f:
        line = line.rstrip()
        tskv = tskv_parse(line)
        access[tskv['trace.id']].append(tskv['grpc.code'])

for trace, couples in info.iteritems():
    try:
        last_try = sorted(couples.iteritems(), key=lambda (x, y): y['ts'])[-1][1]
    except KeyError:
        print "ERROR", trace, couples

    if all([x == 'Unknown' for x in access[trace]]):
        continue

    if last_try.get('COMMIT', False):
        continue

    # print json.dumps(couples, indent=4)
    for c, data in couples.iteritems():
        print data['log_line']

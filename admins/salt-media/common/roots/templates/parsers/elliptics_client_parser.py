#!/usr/bin/env python

from __future__ import print_function
import re
import json
import logging
import fileinput
from collections import defaultdict, Counter


logger = logging.getLogger(__name__)


# Log format: %timestamp% %trace_id%/%thread_id%/%process_id% %LOG_LEVEL%: %log message%
example_finished = """2017-03-06 16:13:35.981908 35aaa77026c1c5d9/265733/265584 INFO: 5bfa702f75b1: WRITE_NEW: finished: groups: [203529, 319777], trans: {203529: 8905806, 319777: 8905807}, status: {203529: 0, 319777: 0}, json-size: { 203529: 62, 319777: 62}, data-size: {203529: 0, 319777: 0}, total_time: 179406, attrs: [trace_id: f0d97d4764301e34, component: elliptics] """
finished_re = re.compile(
    r'(?P<time>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d+) '
    r'(?P<trace_id>[0-9a-z]+).(?P<thread_id>[0-9]+).(?P<process_id>[0-9]+) '
    r'(?P<loglevel>[A-Z]+): '
    r'[0-9a-z:]+ '  # some other id?
    r'(?P<command>[A-Z_]+): '
    r'finished: '
    r'(?P<keyvalues>.+), '
    r'''(?P<attrs>(attrs|'component').+)'''
)


example_destruction = """2017-03-07 14:01:25.991721 111c94d292e8e36a 27576/27566 INFO: 1791:efd8643691ee...d89b197e29a6: REMOVE: destruction trans: 8923611, st: 2a02:6b8:0:821::96:1025/4655, cflags: 0x204 [destroy|reply], wait-ts: 10, stall: 0, time: 1554, started: 2017-03-07 14:01:25.990143, cached status: -2, 'component': 'elliptics'"""
destruction_re = re.compile(
    r'(?P<time>\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d+) '
    r'(?P<trace_id>[0-9a-z]+).(?P<thread_id>[0-9]+).(?P<process_id>[0-9]+) '
    r'(?P<loglevel>[A-Z]+): '
    r'[0-9a-z:\.]+ '  # some other id?
    r'(?P<command>[A-Z_]+): '
    r'destruction '
    r'(?P<keyvalues>.+)'
)


def parse_finished_kv(kv_str):
    # more error prone but way faster then yaml.load
    kv_str = re.sub(r'''["']?([\w_-]+)['"]?\s*:''', '"\g<1>":', '{' + kv_str + '}')
    kv_str = re.sub("'", '"', kv_str)
    try:
        return json.loads(kv_str)
    except Exception:
        logger.error('Failed to parse kv: {0}'.format(kv_str))
        raise


def parse_destruction_kv(kv_str):
    data = kv_str.split()
    return {
        'status': int(data[18].strip(',')),  # cached status: -2
        'time': int(data[12].strip(',')),    # time: 1235
    }


def parse_finished(m):
    data = parse_finished_kv(m.group('keyvalues'))
    for group in data['groups']:
        group = str(group)
        yield {
            'command': m.group('command'),
            'status': data['status'][group],
            'time': data['total_time'] / 1e6,
            'json-size': data['json-size'][group],
            'data-size': data['data-size'][group],
        }


def parse_destruction(m):
    data = parse_destruction_kv(m.group('keyvalues'))
    yield {
        'command': m.group('command'),
        'status': data['status'],
        'time': data['time'] / 1e6,
    }


parsers = [
    ('finished', finished_re, parse_finished),
    ('destruction', destruction_re, parse_destruction),
]


def parse_line(line):
    for name, matcher, parser in parsers:
        m = matcher.match(line)
        if m:
            logger.debug('Matched {0} for line {1}'.format(name, line))
            return parser(m)
    assert ': finished:' not in line, 'Regex fail on ' + line
    return []


def collect_metrics(input_stream):
    def metric():
        d = defaultdict(int)
        d['timings'] = Counter()
        return d
    metrics = defaultdict(metric)

    for line in input_stream:
        for op in parse_line(line):
            m = metrics[op['command']]
            logger.debug('Got "{0}" from line "{1}"'.format(op, line))
            m['status.{0}'.format(op['status'])] += 1
            m['timings']['{0:.3f}'.format(op['time'])] += 1
            if 'json-size' in op:
                m['size.json'] += op['json-size']
            if 'data-size' in op:
                m['size.data'] += op['data-size']

    return dict(metrics)


def format_output(metrics):
    res = []
    for prefix, metric in metrics.items():
        for name, value in metric.items():
            metric_name = '{0}_{1}'.format(prefix, name)
            if isinstance(value, Counter):
                value = " ".join(map(lambda t: "%s@%s" % t, sorted(value.items())))
                metric_name = '@' + metric_name

            res.append('{0} {1}'.format(metric_name, value))
    return '\n'.join(sorted(res))


def main():
    stream = fileinput.input()
    # stream = [example_destruction, example_finished]
    print(format_output(collect_metrics(stream)))


if __name__ == '__main__':
    logging.basicConfig(format='[%(asctime)s][%(name)s][%(levelname)s]: %(message)s', level=logging.ERROR)
    main()

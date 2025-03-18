import argparse
import json
import numpy as np
import pandas as pd

from antiadblock.tasks.tools.logger import create_logger

logger = create_logger('performance_shoots_analyzer')


def makestats(filenames):
    data = dict()
    for filename in filenames.split(','):
        datafile = open(filename, 'r')
        caches = []
        cntgood = 0
        cntbad = 0
        for line in datafile.readlines():
            try:
                obj = json.loads(line)
                if 'duration' not in obj:
                    continue
                cntgood += 1
                action = obj['action']
                dur = obj['duration']
                if action not in data:
                    data[action] = []
                data[action].append(dur)
                if action == 'cache_get':
                    caches.append(obj)
            except:
                cntbad += 1
        datafile.close()
        logger.info(f'{filename}: {cntgood} good, {cntbad} bad lines')
        caches.sort(key=lambda x: x['duration'], reverse=True)
        with open(filename + '_cachestat', 'w') as f:
            for o in caches:
                f.write(str(o['duration']) + '\t' + o['service_id'] + '\t' + o['url'] + '\n')

    df = pd.DataFrame(columns=['action', 'count', 'p50', 'p75', 'p90', 'p95', 'p99', 'p99.9'])
    for k in data.keys():
        p50, p75, p90, p95, p99, p999 = np.percentile(data[k], (50, 75, 90, 95, 99, 99.9))
        df.loc[len(df)] = [k, len(data[k]), p50, p75, p90, p95, p99, p999]

    return df


if __name__ == '__main__':
    pd.set_option('display.max_columns', None)
    pd.set_option('display.max_rows', None)
    pd.set_option('display.max_colwidth', None)

    parser = argparse.ArgumentParser(description='Performance shoots log analyzer')
    parser.add_argument('--shoot-0', type=str, required=True, help='path to the first shooting log')
    parser.add_argument('--shoot-1', type=str, help='path to the second shooting log')
    args = parser.parse_args()

    logger.info('Reading logs...')
    stats = []
    if args.shoot_0:
        df = makestats(args.shoot_0)
        stats.append((args.shoot_0, df))

    if args.shoot_1:
        df = makestats(args.shoot_1)
        stats.append((args.shoot_1, df))

    for (fname, df) in stats:
        print()
        print(fname + ':')
        print(df.to_string())

    if len(stats) == 2:
        diff = pd.DataFrame(columns=['action', 'count_0', 'count_1', 'p99_0', 'p99_1', 'diff (%)'])
        df0 = stats[0][1]
        df1 = stats[1][1]
        for i in range(len(df0)):
            r0 = df0.loc[i]
            r1 = None
            for j in range(len(df1)):
                if df1.loc[j][0] == r0[0]:
                    r1 = df1.loc[j]
                    break
            if r1 is None:
                logger.error(f'action {r0[0]} not found in shoot-1 logs')
                continue
            diff.loc[i] = [r0[0], r0[1], r1[1], r0[-2], r1[-2], (r1[-2] * 1.0 / r0[-2] - 1.0) * 100.0]
        print()
        print('diff:')
        print(diff.to_string())

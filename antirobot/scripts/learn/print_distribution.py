#!/usr/bin/env python
from __future__ import division
from bisect import bisect
import sys

def calculate_classes_distribution(data, thresholds, get_absolute = False):
    "data = ((class, score), ...)"
    classes = 0, 1
    segments_number = len(thresholds) - 1
    scores = sorted(thresholds)
    count = [[0] * (len(thresholds) + 1) for i in classes]
    total = [0] * len(classes)
    for class_, score in data:
        count[class_][bisect(scores, score)] += 1
        total[class_] += 1
    return count if get_absolute else [
            map(lambda cnt: cnt/class_total, class_count)
                for (class_total, class_count) in zip(total, count)]

def calculate_recognition_statistics(data, thresholds):
    counts = [[[0, 0], [0, 0]] for threshold in thresholds]
    thresholds_counts = zip(thresholds, counts)
    for class_, score in data:
        for (threshold, count) in thresholds_counts:
            count[class_][0 if score < threshold else 1] += 1
    return counts


def data_reader(f):
    for line in f:
        fields = line.split()
        try:
            yield int(float(fields[1]) + 0.5), float(fields[4])
        except IndexError:
            pass

command_shortcuts = {
    'rel' : 'relative',
    'abs' : 'absolute',
    'stats' : 'statistics',
    'stats2' : 'statistics2',
    'stabs' : 'absolute_statistics',
}

def _float_list_to_str(floatlist):
    return '\t'.join(map('{0:f}'.format, floatlist))

def _int_list_to_str(intlist):
    return '\t'.join(map('{0:d}'.format, intlist))

def f_measure(beta, precision, recall):
    if precision < 1e-8 and recall < 1e-8:
        return 0
    return (1 + beta * beta) * precision * recall / (beta * beta * precision + recall)

def _f_measure(stats):
    return f_measure(0.5, stats[1][1] / max(1.0, stats[1][1] + stats[0][1]),
                          stats[1][1] / max(1.0, stats[1][1] + stats[1][0]))

def main():
    command = sys.argv[1].lstrip('-') if len(sys.argv) > 1 else 'relative'
    command = command_shortcuts.get(command, command)
    if command in ('relative', 'absolute'):
        distribution = calculate_classes_distribution(data_reader(sys.stdin), map(float, sys.argv[2:]), command == 'absolute')
        print('\n'.join('\t'.join(map(str, density)) for density in zip(*distribution)))
    elif command == 'statistics':
        all_stats = calculate_recognition_statistics(data_reader(sys.stdin), map(float, sys.argv[2:]))
        total_humans = sum(all_stats[0][0])
        total_robots = sum(all_stats[0][1])
        print('border\t' + '\t'.join(sys.argv[2:]))
        print('true neg\t' + _float_list_to_str(stats[0][0] / max(1.0, total_humans) for stats in all_stats))
        print('true pos\t' + _float_list_to_str(stats[1][1] / max(1.0, total_robots) for stats in all_stats))
        print('false pos\t' + _float_list_to_str(stats[0][1] / max(1.0, total_humans) for stats in all_stats))
        print('false neg\t' + _float_list_to_str(stats[1][0] / max(1.0, total_robots) for stats in all_stats))
        print('precision\t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[0][1]) for stats in all_stats))
        print('recall  \t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[1][0]) for stats in all_stats))
        print('f-measure\t' + _float_list_to_str(map(_f_measure, all_stats)))
        print('total: %d, robots: %d (%0.2f%%)' % (total_humans + total_robots, total_robots, float(total_robots) * 100.0 / (total_humans + total_robots)));
    elif command == 'absolute_statistics':
        all_stats = calculate_recognition_statistics(data_reader(sys.stdin), map(float, sys.argv[2:]))
        total_humans = sum(all_stats[0][0])
        total_robots = sum(all_stats[0][1])
        print('border\t' + '\t'.join(sys.argv[2:]))
        print('true neg\t' + _int_list_to_str(stats[0][0] for stats in all_stats))
        print('true pos\t' + _int_list_to_str(stats[1][1] for stats in all_stats))
        print('false pos\t' + _int_list_to_str(stats[0][1] for stats in all_stats))
        print('false neg\t' + _int_list_to_str(stats[1][0] for stats in all_stats))
        print('precision\t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[0][1]) for stats in all_stats))
        print('recall  \t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[1][0]) for stats in all_stats))
        print('f-measure\t' + _float_list_to_str(map(_f_measure, all_stats)))
        print('total: %d, robots: %d (%0.2f%%)' % (total_humans + total_robots, total_robots, float(total_robots) * 100.0 / (total_humans + total_robots)));
    elif command == 'statistics2':
        all_stats = calculate_recognition_statistics(data_reader(sys.stdin), map(float, sys.argv[2:]))
        total_humans = sum(all_stats[0][0])
        total_robots = sum(all_stats[0][1])
        total = total_humans + total_robots
        print('border\t' + '\t'.join(sys.argv[2:]))
        print('true neg\t' + _float_list_to_str(stats[0][0] / max(1.0, total) for stats in all_stats))
        print('true pos\t' + _float_list_to_str(stats[1][1] / max(1.0, total) for stats in all_stats))
        print('false pos\t' + _float_list_to_str(stats[0][1] / max(1.0, total) for stats in all_stats))
        print('false neg\t' + _float_list_to_str(stats[1][0] / max(1.0, total) for stats in all_stats))
        print('precision\t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[0][1]) for stats in all_stats))
        print('recall  \t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[1][0]) for stats in all_stats))
        print('precision\t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[0][1]) for stats in all_stats))
        print('recall  \t' + _float_list_to_str(stats[1][1] / max(1.0, stats[1][1] + stats[1][0]) for stats in all_stats))
        print('f-measure\t' + _float_list_to_str(map(_f_measure, all_stats)))
        print('total: %d, robots: %d (%0.2f%%)' % (total_humans + total_robots, total_robots, float(total_robots) * 100.0 / (total_humans + total_robots)));
    else:
        print('Usage: ' + sys.argv[0] + ' [command = relative] [command_options]')
        print('Commands:')
        print('  rel[ative] [points_number]    -- distribution density function')
        print('  abs[olute] [points_number]    -- number of samples in each segment')
        print('  stat[istic]s <threshold> ...  -- recognition statistics for given threshold')
        print('  stabs/absoulte_statistics <threshold> ...  -- absolute numbers for recognition statistics for given threshold')

if __name__ == '__main__':
    main()


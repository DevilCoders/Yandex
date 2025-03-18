import collections
import logging
import random

import data


class Value(object):
    def __init__(self, data, target):
        self.data = data
        self.deviation = sum(data) - target

    def replace(self, index, value):
        old_value = self.data[index]
        self.deviation = self.deviation + value - old_value
        self.data[index] = value
        return old_value

    def __repr__(self):
        return str(self.data) + ', dev: ' + str(self.deviation)


def score(*values):
    return sum(value.deviation ** 2 for value in values)


def interchange(o1, o2):
    old_score = score(o1, o2)
    for i1, v1 in enumerate(o1.data):
        for i2, v2 in enumerate(o2.data):
            old_v1 = o1.replace(i1, v2)
            old_v2 = o2.replace(i2, v1)
            new_score = score(o1, o2)
            if new_score < old_score:
                 return new_score - old_score  # maybe find the best replacement, not the first
            else:
                o1.replace(i1, v1)
                o2.replace(i2, v2)

    raise RuntimeError('stalled')


def opt(values, steps=200):
    for step in xrange(steps):
        values.sort(key=lambda x: x.deviation)
        found = False
        for min_index, min_value in enumerate(values):
            for max_index in xrange(len(values) - 1, min_index, -1):
                max_value = values[max_index]
                logging.debug('min: %r, max: %r', min_value, max_value)
                try:
                    interchange(min_value, max_value)
                    found = True
                    break
                except RuntimeError:
                    pass
        if not found:
            values.sort(key=lambda x: x.deviation)
            print str(values)
            return


def main():
    logging.basicConfig(level=logging.DEBUG)

    # data = [float(x) for x in range(100)]
    # random.shuffle(data)
    # target = 2 * sum(data) / len(data)
    powers = data.get_tier1()
    target = 3 * sum(powers) / len(powers)
    print len(powers), target
    random.shuffle(powers)
    values = [Value([powers[i], powers[i+1], powers[i+2]], target) for i in xrange(0, len(powers), 3)]
    opt(values)
    for value in values:
        print value.data[0], value.data[1], value.data[2], value.deviation


if __name__ == '__main__':
    main()

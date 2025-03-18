# coding: utf-8
import json, random, copy
import parse_yp_result


def yp_instances(yp_group):
    for host, count in yp_group.iteritems():
        for i in range(count):
            yield host, i


def gencfg_instance(yp_instance, port, power, group):
    return '{}:{}:{}:{}'.format(yp_instance[0], port + yp_instance[1] * 8, power, group)


def replace_intl2(intlookup, yp_group, int_group):
    intlookup = copy.deepcopy(intlookup)
    instances = list(yp_instances(yp_group))
    random.shuffle(instances)
    int_instances = list(yp_instances(int_group))
    random.shuffle(int_instances)
    print intlookup[0]
    for intl2_group in intlookup[1:]:
        print 'replace', len(intl2_group[0]), 'intl2s'
        intl2_group[0] = [gencfg_instance(instance, INTL2_PORT, '160.000', 'SAS_WEB_INTL2') for instance in instances[:len(intl2_group[0])]]
        instances = instances[len(intl2_group[0]):]
        for x in intl2_group[1:]:
           for y in x:
               y[1] = [gencfg_instance(int_instance, INT_PORT, '120.000', 'SAS_WEB_TIER1_JUPITER_INT')
                       for int_instance in int_instances[:len(y[1])]]
               int_instances = int_instances[len(y[1]):]
    assert not instances  # exhausted
    assert not int_instances
    return intlookup


def dump_hosts(yp_group):
    return '\n'.join(sorted(yp_group.keys()))


def dump_instances(yp_group, port, power):
    return '\n'.join('\n'.join('{}:{} {}'.format(host, port + index * 8, power) for index in range(count)) for host, count in sorted(yp_group.items()))

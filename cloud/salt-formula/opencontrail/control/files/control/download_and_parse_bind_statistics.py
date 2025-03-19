#!/usr/bin/env python3

import requests
from lxml import etree
import logging
import json


BIND_STATISTICS_PORT = 8053
BIND_STATISTICS_URL = 'http://127.0.0.1:{}'.format(BIND_STATISTICS_PORT)
MEMORY_STATISTICS_URL = BIND_STATISTICS_URL + "/xml/v3/mem"
SERVER_STATISTICS_URL = BIND_STATISTICS_URL + "/xml/v3/server"

DOWNLOAD_TIMEOUT = 60  # seconds


logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
logger = logging.getLogger('bind_statistics_parser')


def extract_counters_metrics(tree):
    """
    Extract metrics from bind's counters (such a
        <counters type="zonestat">
            <counter name="NotifyOutv4">0</counter>
            <counter name="NotifyOutv6">0</counter>
            <counter name="NotifyInv4">0</counter>
            <counter name="NotifyInv6">0</counter>
            <counter name="NotifyRej">0</counter>
            <counter name="SOAOutv4">0</counter>
            <counter name="SOAOutv6">0</counter>
            <counter name="AXFRReqv4">0</counter>
            <counter name="AXFRReqv6">0</counter>
            <counter name="IXFRReqv4">0</counter>
            <counter name="IXFRReqv6">0</counter>
            <counter name="XfrSuccess">0</counter>
            <counter name="XfrFail">0</counter>
        </counters>
    )
    """
    counters = tree.xpath('/statistics/server/counters')
    for counters_group in counters:
        counters_type = counters_group.attrib['type']
        counters = counters_group.xpath('counter')
        for counter in counters:
            counter_name = counter.attrib['name']
            logger.info("Processing counter {}.{}".format(counters_type, counter_name))

            labels = {
                'type': 'counter',
                'counter_type': counters_type,
                'counter_name': counter_name
            }
            value = counter.text

            yield ('rate', labels, value)


def extract_memory_metrics(tree):
    """
    Extract memory metrics from contexts (such a
        <context>
            <id>0x7fa9e9828180</id>
            <name>main</name>
            <references>413231</references>
            <total>1896473949</total>
            <inuse>1538394092</inuse>
            <maxinuse>1600289746</maxinuse>
            <blocksize>-</blocksize>
            <pools>127130</pools>
            <hiwater>0</hiwater>
            <lowater>0</lowater>
        </context>
    ) and from summary (such a
         <summary>
            <TotalUse>6855333102</TotalUse>
            <InUse>3303301430</InUse>
            <BlockSize>0</BlockSize>
            <ContextSize>1414734928</ContextSize>
            <Lost>0</Lost>
         </summary>
    )
    """

    contexts_statistics_by_name = {}

    contexts = tree.xpath('/statistics/memory/contexts/context')
    for context in contexts:
        context_name = context.xpath('name')[0].text

        if context_name not in contexts_statistics_by_name:
            contexts_statistics_by_name[context_name] = {
                'total': 0,
                'inuse': 0,
                'pools': 0,
                'maxinuse': 0,
            }

        for parameter_name in contexts_statistics_by_name[context_name].keys():
            value = int(context.xpath(parameter_name)[0].text)
            contexts_statistics_by_name[context_name][parameter_name] += value

    for context_name, values in contexts_statistics_by_name.items():
        for parameter_name, value in values.items():
            labels = {
                'type': 'memory_context',
                'context_name': context_name,
                'parameter_name': parameter_name
            }
            yield ('gauge', labels, value)

    summary = tree.xpath('/statistics/memory/summary')[0]
    for children in summary.iterchildren():
        parameter_name = children.tag

        labels = {
            'type': 'memory_summary',
            'name': parameter_name,
        }
        value = children.text

        yield ('gauge', labels, value)


def download_bind_statistics_xml(url):
    logger.info('Download bind\'s statistics xml from {}'.format(url))

    r = requests.get(url, timeout=DOWNLOAD_TIMEOUT)
    r.raise_for_status()

    logger.info('Response\'s content length is {} bytes'.format(r.headers['Content-Length']))

    return r.content


def extract_metrics():
    server_xml = etree.fromstring(download_bind_statistics_xml(SERVER_STATISTICS_URL))
    for sensor_type, labels, value in extract_counters_metrics(server_xml):
        yield (sensor_type, labels, value)

    memory_xml = etree.fromstring(download_bind_statistics_xml(MEMORY_STATISTICS_URL))
    for sensor_type, labels, value in extract_memory_metrics(memory_xml):
        yield (sensor_type, labels, value)


def main():
    logging.info('Starting')
    for sensor_type, metric_labels, metric_value in extract_metrics():
        data = {
            'type': sensor_type,
            'labels': metric_labels,
            'value': metric_value,
        }
        print(json.dumps(data))
    logging.info('Finished')


if __name__ == '__main__':
    main()


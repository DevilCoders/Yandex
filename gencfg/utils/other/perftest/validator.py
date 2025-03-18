#!/usr/bin/env python
"""Validate that different basesearchers results"""

import argparse
import json
import math
import os
import re
import requests
import sys
import time


def indent(text, ind=' ' * 4):
    lines = text.split('\n')
    return '\n'.join([ind + line for line in lines])


class TQueryResponse(object):
    def __init__(self, query, answers):
        self.query = query
        self.answers = answers

    @classmethod
    def from_proto(cls, query, proto_response):
        lines = [x.rstrip() for x in proto_response.split('\n')]

        p = re.compile('^\s+DocId: "(.*)"$')
        doc_ids = [p.match(line).group(1) for line in lines if p.match(line)]

        p = re.compile('^\s+SRelevance: (.*)$')
        relevances = [int(p.match(line).group(1)) for line in lines if p.match(line)]

        assert len(doc_ids) == len(relevances)

        return cls(query, {x: y for x, y in zip(doc_ids, relevances)})

    @classmethod
    def from_json(cls, jsoned):
        return cls(jsoned['query'], jsoned['answers'])

    def to_json(self):
        return dict(query=self.query, answers=self.answers)

    def generate_diff(self, rhs, max_diff_relevance=100):
        result = []

        my_extra_docs = set(self.answers.keys()) - set(rhs.answers.keys())
        if my_extra_docs:
            result.append('Left extra:')
            result.extend(['    Doc {}, relevance {}'.format(x, self.answers[x]) for x in my_extra_docs])

        rhs_extra_docs = set(rhs.answers.keys()) - set(self.answers.keys())
        if rhs_extra_docs:
            result.append('Right extra:')
            result.extend(['    Doc {}, relevance {}'.format(x, rhs.answers[x]) for x in rhs_extra_docs])

        diff_docs = set(self.answers.keys()) & set(rhs.answers.keys())
        diff_docs = [x for x in diff_docs if math.fabs(self.answers[x] - rhs.answers[x]) > max_diff_relevance]
        if diff_docs:
            result.append('Different relevance:')
            result.extend(['    Doc {}, left relevance {}, right relevance {}'.format(x, self.answers[x], rhs.answers[x]) for x in diff_docs])

        return len(result) > 0, '\n'.join(result)

    def __str__(self):
        result = []
        result.append('Query <{}>'.format(self.query))
        result.extend('    Doc {}, relevance {}'.format(x, y) for (x, y) in self.answers.iteritems())
        return '\n'.join(result)


def retry_requests_get(url, retries=5):
    # suppress noisy warningis
    requests.packages.urllib3.disable_warnings()

    for retry in range(retries):
        try:
            r = requests.get(url)
            r.raise_for_status()
            return r
        except Exception as e:
            print '[{:.2f}]: Got exception <{}> while processing url <{}>, attempt {} ({})'.format(time.time(), e.__class__, url, retry, str(e))
            if retry == retries - 1:
                raise Exception("Got exception <%s> while processing url <%s> (%s)" % (e.__class__, url, str(e)))


def get_responses(base_url, queries):
    responses = dict()
    if (not base_url.startswith('http://')) and (not base_url.startswith('https://')):  # get responses from file
        with open(base_url) as f:
            for jsoned in json.loads(f.read()):
                response = TQueryResponse.from_json(jsoned)
                assert response.query in queries
                responses[response.query] = response
    else:
        for query in queries:
            query_url = '{}/yandsearch?{}&ms=proto&hr=da'.format(base_url, query)
            response = TQueryResponse.from_proto(query, retry_requests_get(query_url).text.encode('utf8'))
            responses[response.query] = response

    return responses


def main(options):
    with open(options.queries_file) as f:
        queries = [x.strip() for x in f.readlines()]

    left_responses = get_responses(options.left, queries)
    right_responses = get_responses(options.right, queries)

    status = True
    for query in queries:
        query_status, query_msg = left_responses[query].generate_diff(right_responses[query])
        status &= query_status
        if query_status:
            print 'Query <{}>'.format(query)
            print indent(query_msg)

    if options.dump_dir is not None:
        with open(os.path.join(options.dump_dir, 'left.result'), 'w') as f:
            f.write(json.dumps([x.to_json() for x in left_responses.itervalues()], indent=4))
        with open(os.path.join(options.dump_dir, 'right.result'), 'w') as f:
            f.write(json.dumps([x.to_json() for x in left_responses.itervalues()], indent=4))

    return int(not status)


def get_parser():
    parser = argparse.ArgumentParser(description='Compare two basesearchers responses')
    parser.add_argument('--left', type=str, required=True,
                        help='Obligatory. Url to first compare basesearch')
    parser.add_argument('--right', type=str, required=True,
                        help='Obligatory, Url to second compare basesearch')
    parser.add_argument('-q', '--queries-file', type=str, required=True,
                        help='Obligatory. File with queries')
    parser.add_argument('--dump-dir', type=str, default=None,
                        help='Optional. If specified, dump results as <left.result> <right.result> in specified dir')

    return parser


if __name__ == '__main__':
    options = get_parser().parse_args()

    status = main(options)

    sys.exit(status)

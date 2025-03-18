#!/usr/bin/env python
# -*- coding: utf-8 -*-
import argparse
import json
import base64
import hashlib
import yt.wrapper as yt


class Mapper(object):
    def __init__(self, context, reqid_column):
        # type: (dict) -> None
        self.context = json.dumps(context)
        self.reqid_column = reqid_column

    def __call__(self, row):
        result = json.loads(self.context)
        text = row.get('text', '')
        result[0]['results'][0]['params']['text'] = [text]
        result[0]['results'][1]['text'] = text
        result[0]['results'][1]['user_text'] = text
        if row.get('lr'):
            lr = int(row['lr'])
            result[0]['results'][0]['params']['lr'] = [lr]
            result[0]['results'][1]['lr']['id'] = lr
        if self.reqid_column is not None:
            reqid = str(row[self.reqid_column])
        else:
            reqid = base64.b64encode(hashlib.sha256(str(row)).digest())
        result[0]['results'][1]['reqid'] = reqid
        result_row = {
            'prepared_request': json.dumps(result, ensure_ascii=False).encode('utf-8'),
            'reqid': reqid,
        }
        if row.get('__query_hash__', None) is not None:
            result_row['__query_hash__'] = row['__query_hash__']
        yield result_row


def main():
    parser = argparse.ArgumentParser(
        description='Maps table rows with cgi params to apphost context',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument('-i', '--input', required=True, help='Input table')
    parser.add_argument('-o', '--output', required=True, help='Output table')
    parser.add_argument('-c', '--context', required=True, help='File with initial context')
    parser.add_argument('-j', '--job-count', type=int, help='Yt job count')
    parser.add_argument('-r', '--reqid-column', help='Column with reqid')
    args = parser.parse_args()

    yt.config.config['pickling']['module_filter'] = lambda module: 'hashlib' not in getattr(module, '__name__', '')
    yt.config['spec_defaults']['max_failed_job_count'] = 0
    yt.config['spec_defaults']['max_failed_job_count'] = 0
    context_template = json.load(open(args.context, 'r'))
    mapper = Mapper(context_template, args.reqid_column)
    yt.create('table', args.output, recursive=True)
    yt.run_map(mapper, args.input, args.output, format='json', job_count=args.job_count)


if __name__ == '__main__':
    main()

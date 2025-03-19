#!/usr/bin/env python3
import re

from log_parser import LogParser

key_pattern = re.compile(r'^(?P<key>\d+;\d+;\d+;)')
start_record_pattern = re.compile(key_pattern.pattern + '# Time:')


class SlowQueryLogParser(LogParser):
    def __init__(self):
        super().__init__()
        self.origin = 'mysql_slow_query'
        # self.input_date_format = '%Y-%m-%dT%H:%M:%S.%f'

    def is_record_start(self, s):
        return start_record_pattern.match(s)

    def parse_record(self, record: str):
        lines = record.split('\n')
        result = {}
        query = []
        key = ''
        for line in lines:
            if not line:
                continue
            key_match = key_pattern.match(line)
            if not key_match:
                raise Exception("line without a key: >{}<".format(line))
            key = key_match.group('key')
            line = line[key_match.end() :]  # cut the key
            if line.startswith('#'):
                result.update(self._extract_params(line[1:]))
            else:
                query.append(line)
        result['key'] = key
        result['query'] = ' '.join(query)
        result['raw'] = self.extract_raw(record)
        return result

    def _parse_param(self, field, value):
        if field in ('last_errno', 'killed', 'rows_sent', 'rows_examined', 'rows_affected', 'bytes_sent'):
            return {field: int(value[0])}
        if field in ('query_time', 'lock_time'):
            return {field: float(value[0])}
        if field in ('time'):
            return {'datetime': value[0]}
        if field == 'user@host' and len(value) == 4:
            raw_user = value[0]
            index = raw_user.find('[')
            username = raw_user[index + 1 : len(raw_user) - 1]
            return {'user': username, 'hostname': value[2]}
        return {field: str.join('', value)}

    def _extract_params(self, line):
        tokens = line.split()
        result = {}
        token_list = []
        prev_token = None
        last_idx = len(tokens) - 1
        for idx, t in enumerate(tokens):
            # next token found
            if t.endswith(':'):
                if prev_token is not None:
                    result.update(self._parse_param(prev_token, token_list))
                    token_list = []
                prev_token = t[:-1].lower()
            else:
                token_list.append(t)
            # last value if needed
            if idx == last_idx and prev_token is not None and not t.endswith(':'):
                result.update(self._parse_param(prev_token, token_list))
        return result


def main():
    parser = SlowQueryLogParser()
    parser.run()


if __name__ == '__main__':
    main()

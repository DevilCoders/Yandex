#!/usr/bin/python -u

import re
import json
import sys

INT_GROUPS = ["unixtime", "status", "duration", "grpc_status_code"]
IS_GRPC = False

# line = '1282320;321;68052;"2018-09-05T11:44:13.493Z+0000" 1536147853 "envoy" "gateway" "::1" ' \
#        '"28ece33d-57d6-434f-8bf3-a419a2cb51b6" "localhost" "/endpoints" "HTTP/1.1" "POST" "-" "-" 200 1 ' \
#        '"-" "-" "-" "-" "/endpoints" "/endpoints" "0" ""'

LOG_PATTERN = re.compile(
    r'^\d+;\d+;\d+;"(?P<timestamp>.+?)"\s(?P<unixtime>[0-9]+?)\s"(?P<app>\w+?)"\s"(?P<client_app>\w+?)"\s'
    r'"(?P<remote_ip>.+?)"\s"(?P<request_id>.+?)"\s"(?P<authority>.+?)"\s'
    r'"(?P<request_uri>.+?)"\s"(?P<type>.+?)"\s"(?P<request_method>\w+?)"\s'
    r'"(?P<client_request_id>.+?)"\s"(?P<client_trace_id>.+?)"\s(?P<status>\d+?)\s'
    r'(?P<duration>\d+?)\s"(?P<request>.+?)"\s"(?P<request_raw>.+?)"\s"(?P<response>.+?)"\s'
    r'"(?P<response_raw>.+?)"\s"/(?P<grpc_service>.+?)'
    r'(?:(?:/.*"?)|"?)\s"(?:/.*?)?/(?P<grpc_method>.*?)"\s'
    r'"(?P<grpc_status_code>.+?)"\s"(?P<grpc_status_message>.*?)"')

GROUPS = [k for k, v in sorted(LOG_PATTERN.groupindex.items(), key=lambda x: x[1])]


def if_request_uri(name, value):
    global IS_GRPC
    IS_GRPC = True if "." in value else False
    return {name: value}


def parse_group(name, value):
    if name in INT_GROUPS:
        try:
            value = int(value)
        except ValueError:
            value = 666
        return {name: value}
    else:
        return parse_request_uri_group(name, value)


def parse_request_uri_group(name, value):
    return if_request_uri(name, value) if name == "request_uri" else parse_type_group(name, value)


def parse_type_group(name, value):
    return {name: "grpc"} if name == "type" and IS_GRPC else default(name, value)


def default(name, value):
    return {name: value}


if __name__ == "__main__":
    while True:
        line = sys.stdin.readline()
        if line == '':
            break
        raw_line = line
        try:
            fields = {}
            line = line.split(';', 3)
            data = line[0:3]
            for d in map(lambda x: parse_group(GROUPS[x[0]], x[1]),
                         [(i, j) for i, j in enumerate(LOG_PATTERN.match(";".join(line)).groups())]):
                for k, v in d.items():
                    fields[k] = v
            print(";".join(data) + ";" + json.dumps(
                fields
            ))
            # with open("/var/log/statbox/parsed.log", 'a') as f:
            #     f.write(
            #         ";".join(data) + ";" + json.dumps(
            #             fields
            #         ) + "\n"
            #     )
        except Exception as e:
            with open("/var/log/statbox/unparsed.log", 'a') as f:
                f.writelines([e.__repr__(), "\n", ";".join(raw_line)])

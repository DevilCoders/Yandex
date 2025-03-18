#!/usr/bin/env python3

import urllib.request
import sys
import time
import glob
import difflib


#production_host  = 'https://cbb.n.yandex-team.ru'
production_host = 'http://cbb-production-yp-2.sas.yp-c.yandex.net:200'
production_result = 'result.txt'

prod_new_host = 'http://cbb-production-yp-2.sas.yp-c.yandex.net:300'
prod_new_result = 'result_new.txt'

test_host = 'http://localhost:3000'
test_result = 'test_result.txt' # + str(int(time.time()))

requests = 'request_urls.txt'

def fetch(host, result_filename):
    with open(result_filename, 'wt') as result_file:
        with open(requests) as urls_file:
            for line in urls_file:
                path = line.strip()
                url = host + path
                print (url, file=sys.stderr)
                for _ in range(3):
                    try:
                        with urllib.request.urlopen(url) as response:
                            content = response.read()
                    except urllib.error.HTTPError:
                        continue
                    break

                print (path, file=result_file)
                print ("Content-Length:", len(content), file=result_file)
                lines = content.decode('utf-8').split('\n')
                lines.sort()
                res = '\n'.join(lines)
                print (res, file=result_file)
                print ("", file=result_file)

fetch(production_host, production_result)
fetch(prod_new_host, prod_new_result)
#last_result = glob.glob("test_result.txt.*")[-1]
fetch(test_host, test_result)

"""
with open(last_result) as prev_f:
    with open(test_result) as new_f:
        diff = difflib.ndiff(prev_f.readlines(), new_f.readlines())
        changes = [l for l in diff if l.startswith('+ ') or l.startswith('- ')]
        for c in changes:
            print(c)
"""

#!/usr/bin/env python
# coding: utf-8

import requests

if __name__ == '__main__':
    s = requests.Session()
    print s.get('http://127.0.0.1/test1?x[]=12&x=%27', verify=False, allow_redirects=False)
    print s.post('http://127.0.0.1/test1?x[]=12&x=%27', data={ 'payload': ('/'*2048*30), 'something': 'XXX'}, verify=False, allow_redirects=False)
    print s.post('http://127.0.0.1/test2?x[]=12&x=%27', data={ 'payload': ('X'*2048*30), 'something': 'test'}, verify=False, allow_redirects=False)


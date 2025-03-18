#!/usr/bin/env python
# coding: utf-8

import requests

def gen1():
    yield ('A'*1043)
    yield ('B'*2048)

def gen2():
    yield ('C'*2048)
    yield ('D'*1043)

if __name__ == '__main__':
    s = requests.Session()
    print s.post('http://127.0.0.1/test1?x[]=12&x=%27', data=gen1(), verify=False, allow_redirects=False)
    print s.post('http://127.0.0.1/test2?x[]=12&x=%27', data=gen2(), verify=False, allow_redirects=False)

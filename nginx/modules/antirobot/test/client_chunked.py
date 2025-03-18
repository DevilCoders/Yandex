#!/usr/bin/env python
# coding: utf-8

import requests

def gen():
    yield ('A'*2048)
    yield ('B'*2048)

if __name__ == '__main__':
   print requests.post('http://127.0.0.1/auth.jsx?x[]=12&x=%27', data=gen(), verify=False, allow_redirects=False)

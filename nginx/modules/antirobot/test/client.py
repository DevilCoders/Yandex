#!/usr/bin/env python
# coding: utf-8

import requests

if __name__ == '__main__':
   print requests.get('http://127.0.0.1/auth.jsx?x[]=12&x=%27', verify=False, allow_redirects=False)
   print requests.post('http://127.0.0.1/auth.jsx?x[]=12&x=%27', data={ 'payload': ('/'*2048*30), 'something': 'XXX'}, verify=False, allow_redirects=False)

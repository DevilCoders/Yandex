#!/usr/bin/env python

# PhConfig API Request example:
# curl https://phconfig.search.yandex.net/?args=<string>&fmt=json


import urllib2
import time
import json
import urllib


# Exceptions
class ModuleException(Exception):
    ''' Base exception class. '''
    pass


class EmptyString(ModuleException):
    pass


class HTTPError(ModuleException):
    pass


class Module(object):
    ''' PhConfig service resolver module.
    Pass all arguments as you will do with local phconfig binary as a single string.
    Examples:
        "F orca"
        "s youtube.ru"
    '''

    def __init__(self):
        self.urlTmpl = "http://phconfig.n.yandex-team.ru/?args={}&fmt=json"
        self.retryCount = 4
        self.retryTimeout = 0.1

    def __GetHTTPData(self, url):
        ''' str -> str
        Obtain data via HTTP.
        '''
        request = urllib2.Request(url)
        for step in range(self.retryCount):
            try:
                response = urllib2.urlopen(request)
            except urllib2.HTTPError as error:
                if step == (self.retryCount - 1):
                    raise HTTPError("{}\n{} ({})".format(url, error.reason, error.code))
                time.sleep(self.retryTimeout)
            except urllib2.URLError as error:
                raise urllib2.URLError("{} (for {})".format(error.reason, url))
            else:
                break
        return response.read()

    def __GetData(self, string):
        ''' str -> iterator
        Retrive JSON from server and represent as dict()
        '''
        req = urllib.quote(string)
        url = self.urlTmpl.format(req)
        data = json.loads(self.__GetHTTPData(url))
        return iter(data)

    def __call__(self, string):
        ''' str -> tuple
        Entry Point.
        '''
        if not string:
            raise EmptyString("Request string can not be empty.")
        return tuple(self.__GetData(string))

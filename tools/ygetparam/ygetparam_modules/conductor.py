#!/usr/bin/env python

# Conductor API Request example:
# curl https://c.yandex-team.ru/api/groups2hosts/<group_name>

import urllib2
import time


# Exceptions
class ModuleException(Exception):
    ''' Base exception class. '''
    pass


class EmptyString(ModuleException):
    pass


class HTTPError(ModuleException):
    pass


class Module(object):
    ''' Conductor's group resolver module '''

    def __init__(self):
        self.urlTmpl = "https://c.yandex-team.ru/api/groups2hosts/{}"
        self.retryCount = 4
        self.retryTimeout = 0.1

    def __GetHTTPData(self, url):
        ''' str -> tuple
        Obtain data via HTTP
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

    def __ResolveHosts(self, group):
        ''' str -> generator
        Return list of hosts for specified configuration
        '''
        url = self.urlTmpl.format(group)
        data = self.__GetHTTPData(url)
        return (item for item in data.split('\n')[:-1])

    def __call__(self, string):
        ''' str -> tuple
        Entry point.
        '''

        if not string:
            raise EmptyString("Request string can't be empty")

        hosts_tuple = tuple(self.__ResolveHosts(string))
        return hosts_tuple

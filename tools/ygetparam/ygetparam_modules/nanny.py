#!/usr/bin/env python

# Nanny API Request example:
# curl https://nanny.yandex-team.ru/v2/services/<service_name>/current_state/instances/


import urllib2
import time
import json


# Exceptions
class ModuleException(Exception):
    ''' Base exception class. '''
    pass


class UndefinedReqType(ModuleException):
    pass


class EmptyString(ModuleException):
    pass


class HTTPError(ModuleException):
    pass


class ServiceIsNotReady(ModuleException):
    pass


class Module(object):
    ''' Nanny service resolver module '''

    def __init__(self):
        self.urlTmpl = "https://nanny.yandex-team.ru/v2/services/{}/current_state/instances"
        self.urlStateTmpl = "https://nanny.yandex-team.ru/v2/services/{}/current_state"
        self.retryCount = 4
        self.retryTimeout = 0.1

        self.PREFIX_HOSTS = "hosts:"
        self.PREFIX_FQDNS = "fqdns:"
        self.PREFIX_PORT = "port:"

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

    def __GetData(self, service):
        ''' str -> dict
        Retrive JSON from server and represent as dict()
        '''
        url = self.urlTmpl.format(service)
        data = json.loads(self.__GetHTTPData(url), parse_int=str)
        if not data["result"]:
            stateUrl = self.urlStateTmpl.format(service)
            data = json.loads(self.__GetHTTPData(stateUrl))
            currentState = data["content"]["summary"]["value"]
            raise ServiceIsNotReady("Service is in {} state.".format(currentState))
        return data

    def __ResolvHosts(self, service):
        ''' str -> generator
        Return list of hosts for specified configuration.
        '''
        if not service:
            raise EmptyString("Service name can not be empty.")
        data = self.__GetData(service)
        return (item["hostname"] for item in data["result"])

    def __ResolvPort(self, service):
        ''' str -> str
        Return port number from the first available host.
        Expected that post is always the same.
        '''
        if not service:
            raise EmptyString("Service name can not be empty.")
        data = self.__GetData(service)
        return data["result"][0]["port"]

    def __ResolvHostsFQDN(self, service):
        ''' str -> generator
        Return list of containers' fqdn's (Fully Qualified Domain Name) for specified configuration.
        '''
        if not service:
            raise EmptyString("Service name can not be empty.")
        data = self.__GetData(service)
        return (item["container_hostname"] for item in data["result"])

    def __call__(self, string):
        ''' str -> tuple
        Entry Point.
        '''

        if string.startswith(self.PREFIX_HOSTS):
            retVal = self.__ResolvHosts(string[len(self.PREFIX_HOSTS):])
            retVal = tuple(retVal)
        elif string.startswith(self.PREFIX_FQDNS):
            retVal = self.__ResolvHostsFQDN(string[len(self.PREFIX_FQDNS):])
            retVal = tuple(retVal)
        elif string.startswith(self.PREFIX_PORT):
            retVal = self.__ResolvPort(string[len(self.PREFIX_PORT):])
            retVal = (retVal,)
        else:
            if not string:
                raise EmptyString("Request string can not be empty.")
            raise UndefinedReqType("Unable to retrive type (hosts or port) from string '{}'.".format(string))
        return retVal

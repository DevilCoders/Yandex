#!/usr/bin/env python3
# -*- coding: utf-8 -*-
'''Icecraem backend exceptions'''
import logging
import connexion

class IceCommonException(connexion.ProblemException):
    """Common icecream error"""
    def __init__(self, **kwargs):

        self.type = kwargs.get("type", self.__class__.__name__)
        self.detail = kwargs.get("detail", self.__class__.__doc__)

        kwargs["type"] = self.type
        kwargs["title"] = kwargs.get("title", self.__class__.__doc__)
        kwargs["status"] = kwargs.get("status", 500)
        kwargs["detail"] = self.detail
        super(IceCommonException, self).__init__(**kwargs)

    def __str__(self):
        return "<{}: [{}]>".format(self.type, self.detail)


class IceUnexpectedException(IceCommonException):
    """Resize exception"""


class IceResizeError(IceCommonException):
    """Resize exception"""


class IceMigrateError(IceCommonException):
    """Migration exception"""


class IceMigrateModeUnsupportedError(IceMigrateError):
    """Unsupported migration mode"""
    def __init__(self, detail=None):
        super(IceMigrateModeUnsupportedError, self).__init__(status=400, detail=detail)


class IceDom0Exception(IceCommonException):
    """dom0 related exceptions"""
    def __init__(self, status=500, detail=None):
        super(IceDom0Exception, self).__init__(status=status, detail=detail)


class IceCreateContainerException(IceCommonException):
    """Container create exception"""
    def __init__(self, status=409, detail=None):
        super(IceCreateContainerException, self).__init__(status=status, detail=detail)


class IceLxdClientError(IceCommonException):
    """Lxd client connection exception"""


def wrap_error_handler(handler):
    """Icecream common_error_handler"""
    def common_error_handler(_, exception):
        """Wrapper take class and exception params"""
        log = logging.getLogger().error
        if isinstance(exception, IceCommonException):
            ext = getattr(exception, "ext", {})
            if ext and ext.get("log_exception", False):
                del ext["log_exception"]
                # if need to log full stack trace
                log = logging.getLogger().exception
        else:
            exception = IceUnexpectedException(
                detail=str(exception),
                ext={"caused_by": exception},
            )
        log(exception)
        return handler(exception)
    return common_error_handler

# coding: utf-8

from __future__ import division, absolute_import, print_function, unicode_literals

import requests.exceptions

from .tools import dump_raw_request, dump_response, PY_3


class StatfaceClientError(Exception):
    pass


class StatfaceClientFatalError(StatfaceClientError):
    pass


class StatfaceClientRetriableError(StatfaceClientError):
    pass


class StatfaceClientValueError(StatfaceClientFatalError, ValueError):
    pass


class StatfaceHttpResponseError(  # pylint: disable=too-many-ancestors
    StatfaceClientError,
    requests.RequestException
):

    _title = 'Request to statface failed'

    def __init__(self, response, request, original_error=None):
        super(StatfaceHttpResponseError, self).__init__(
            response=response,
            request=request
        )

        self.original_error = original_error

    def __str__(self):
        if self.response is not None:
            resp_dump = dump_response(self.response)
        else:
            resp_dump = 'No response'

        if self.request:
            req_dump = dump_raw_request(self.request)
        else:
            req_dump = 'Empty request'

        original_error_msg = ''
        if PY_3:
            pass
        elif self.original_error is not None:
            original_error_msg = 'Original error: {!s}'.format(self.original_error)
        # else:
        #     original_error_msg = 'No original error'

        message = '{}:\n{}'.format(
            self._title,
            '\n\n'.join((resp_dump, req_dump, original_error_msg))
        )

        return message

    def __repr__(self):
        return "<{cls}(response={resp})>".format(
            cls=self.__class__.__name__,
            resp=self.response,
        )


class StatfaceHttpResponseFatalError(  # pylint: disable=too-many-ancestors
    StatfaceHttpResponseError,
    StatfaceClientFatalError
):

    _title = 'Request to statface failed (non-retriable)'


class StatfaceHttpResponseRetriableError(  # pylint: disable=too-many-ancestors
    StatfaceHttpResponseError,
    StatfaceClientRetriableError
):

    _title = 'Request to statface failed (retriable)'


class StatfaceIncorrectResponseError(StatfaceClientFatalError):
    pass


class StatfaceClientAuthError(StatfaceClientFatalError):
    pass


class StatfaceClientReportError(StatfaceClientFatalError):
    pass


class StatfaceClientDataUploadError(StatfaceClientRetriableError):
    pass


class StatfaceClientDataDownloadError(StatfaceClientRetriableError):
    pass


class StatfaceReportConfigError(StatfaceClientReportError):
    pass


class StatfaceClientAuthConfigError(StatfaceClientFatalError):
    pass


class StatfaceInvalidReportPathError(StatfaceClientReportError):
    pass

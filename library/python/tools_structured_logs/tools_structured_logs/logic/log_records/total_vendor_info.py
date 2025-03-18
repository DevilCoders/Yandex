# coding: utf-8

import six
from monotonic import monotonic

from tools_structured_logs.logic.configuration.library import get_logger
from tools_structured_logs.logic.state import get_state


class TotalVendorInfo(object):
    """
    Пишет в логи общее время выполнения запроса а так же суммарную инфу по каждому из вендоров.
    """
    started_at = None
    code_block_name = None

    def __init__(self, code_block_name, code_block_type, threshold):
        """
        @param code_block_name: имя блока, например "GET /homepage", "my_python_function"
        @param code_block_type: тип, например "http request", "custom function" итп
        @param threshold: msec
        """
        self.code_block_name = code_block_name
        self.code_block_type = code_block_type
        self.threshold = threshold

    def _vendor_summary(self, state):
        """
        @type state: StateItem
        """
        for vendor, info in state.items():
            with get_logger().log_context(execution_time=int(info._milliseconds)):
                get_logger().info(
                    u'"%s" called %d times with total duration of %.1f msec',
                    vendor,
                    info._called_times,
                    info._milliseconds
                )

    def __enter__(self):
        self.started_at = monotonic()
        get_state().put_state()

    def __exit__(self, exc_type, exc_val, exc_tb):
        state = get_state().pop_state()
        execution_time = (monotonic() - self.started_at) * 1000
        if execution_time < self.threshold:
            return
        with get_logger().log_context(execution_time=int(execution_time)):
            self._vendor_summary(state)
            ctx = {}
            if exc_type:
                ctx['unhandled_exception'] = dict(type=repr(exc_type), value=repr(exc_val))
            # "profiling" это блок который присутствует только в финальной записи
            # это требование нужно для того чтобы в YQL выбирать было удобно.
            # запрос предполагается строить как WHERE profiling/final=1
            ctx['profiling'] = dict(
                final=1,
                block_name=self.code_block_name,
                block_type=self.code_block_type,
            )

            with get_logger().log_context(**ctx):
                kwargs = {}
                if exc_type is not None:
                    kwargs['exc_info'] = (exc_type, exc_val, exc_tb)
                get_logger().info(
                    u'%s %s finished in %.1f msec',
                    self.code_block_type, self.code_block_name, execution_time,
                    **kwargs
                )
        return False


class VendorInfoInHttpRequest(TotalVendorInfo):
    @staticmethod
    def request_repr(http_request):
        path = http_request.path
        if six.PY2 and isinstance(http_request.path, str):
            path = http_request.path.decode('UTF-8')
        return u'{method}: {path}'.format(
            method=http_request.method,
            path=path,
        )

    def __init__(self, request, threshold):
        super(VendorInfoInHttpRequest, self).__init__(
            self.request_repr(request),
            code_block_type='http request',
            threshold=threshold,
        )

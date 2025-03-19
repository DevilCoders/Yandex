import logging.handlers

from common.async_helper import AsyncHelper


class AsyncLoggingHandlerMixin(AsyncHelper):
    """
    Logging handler which do all work in another thread and doesn't block main thread.
    """
    def __init__(self, *args, **kwargs):
        super(AsyncLoggingHandlerMixin, self).__init__(*args, **kwargs)
        self.extract_method_to_thread("emit")


class AsyncWatchedFileHandler(AsyncLoggingHandlerMixin, logging.handlers.WatchedFileHandler):
    pass

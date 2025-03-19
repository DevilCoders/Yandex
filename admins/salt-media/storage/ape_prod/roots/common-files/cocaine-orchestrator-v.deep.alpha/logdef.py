import logging

LOG_FORMAT = '%(asctime)-26s %(routine)-22s %(levelname)-10s %(message)s'
LOGLEVEL_DEFAULT = logging.INFO
LOGLEVEL_DEBUG = logging.DEBUG


class ContextAdapter(logging.LoggerAdapter):
    def process(self, msg, kwargs):
        kwargs.setdefault("extra", {}).update(self.extra)
        return(msg, kwargs)

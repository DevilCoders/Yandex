import contextlib
import contextvars
import logging


class MdbLoggerAdapter(logging.LoggerAdapter):
    # context shared between all MdbLoggerAdapter instances
    log_ctx = contextvars.ContextVar('log_ctx')

    def copy_with_ctx(self, **extra):
        if self.extra:
            source_extra = self.extra.copy()
            source_extra.update(extra)
            extra = source_extra
        return MdbLoggerAdapter(self.logger, extra)

    @contextlib.contextmanager
    def context(self, **kwargs):
        """
        Set context to affect all MdbLoggerAdapter instances.
        """
        ctx_extra = self.log_ctx.get({}).copy()
        ctx_extra.update(kwargs)
        token = self.log_ctx.set(ctx_extra)
        yield
        self.log_ctx.reset(token)

    def process(self, msg, kwargs):
        msg, kwargs = super().process(msg, kwargs)
        kwargs_extra = (kwargs['extra'] or {}).copy()
        ctx_extra = self.log_ctx.get({}).copy()
        kwargs_extra.update(ctx_extra)
        kwargs['extra'] = kwargs_extra
        return msg, kwargs

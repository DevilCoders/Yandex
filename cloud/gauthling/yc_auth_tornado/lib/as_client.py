import tornado.gen

import yc_as_client

from yc_auth_tornado import futures


class TornadoYCAccessServiceClient(yc_as_client.YCAccessServiceClient):
    """An asynchronous client for YCAS that returns Tornado futures."""

    def __init__(self, *args, **kwargs):
        self.__client = yc_as_client.YCAccessServiceClient(*args, **kwargs)

    @tornado.gen.coroutine
    def authenticate(self, iam_token=None, signature=None, request_id=None):
        result = yield futures.wrap_future(
            self.__client.authenticate.future(
                iam_token=iam_token,
                signature=signature,
                request_id=request_id,
            )
        )
        raise tornado.gen.Return(result)

    @tornado.gen.coroutine
    def authorize(
        self,
        permission,
        resource_path,
        subject=None, iam_token=None, signature=None,
        request_id=None,
    ):
        result = yield futures.wrap_future(
            self.__client.authorize.future(
                permission=permission,
                resource_path=resource_path,
                subject=subject,
                iam_token=iam_token,
                signature=signature,
                request_id=request_id,
            )
        )
        raise tornado.gen.Return(result)

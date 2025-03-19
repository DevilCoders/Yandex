import grpc
from tornado import gen
from tornado.ioloop import IOLoop

from yc_auth.exceptions import convert_to_yc_auth_error


def wrap_future(grpc_future):
    """Wraps a GRPC future in one that can be yielded by Tornado"""

    tornado_future = gen.Future()
    loop = IOLoop.current()
    grpc_future.add_done_callback(lambda _: loop.add_callback(_set_result, grpc_future, tornado_future))
    return tornado_future


def _set_result(grpc_future, tornado_future):
    """Assigns GRPC future result to Tornado one"""

    try:
        tornado_future.set_result(grpc_future.result())
    except grpc.RpcError as e:
        tornado_future.set_exception(convert_to_yc_auth_error(e))
    except Exception as e:
        tornado_future.set_exception(e)

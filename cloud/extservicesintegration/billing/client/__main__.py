import tornado
import tornado.web
import tornado.ioloop
import random
import string
import uuid
import threading
from collections import deque
from functools import wraps
from threading import Lock

from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import PQStreamingAPI, ConsumerMessageType
from kikimr.public.sdk.python.persqueue.grpc_pq_streaming_api import ProducerConfigurator, ConsumerConfigurator

def singleton_provider(func):
    __singleton_instance = None
    __singleton_lock = threading.Lock()

    @wraps(func)
    def wrapped(*args, **kwargs):
        if not __singleton_instance:
            with __singleton_lock:
                if not __singleton_instance:
                    __singleton_instance = func(*args, **kwargs)
        return __singleton_instance

    return wrapped

class singleton:
    def __init__(self, cls):
        self.__cls = cls
        self.__instance = None

    def __call__(self, *args, **kwds):
        if self.__instance == None:
            self.__instance = self.__cls(*args, **kwds)
        return self.__instance

@singleton
class YaCloudBillingClient:
    _pqapi = None
    _TIMEOUT_SECONDS = 5
    _writer = None
    _current_seq_no = None
    _lock = Lock()
    _in_flight_writes = 0

    def __init__(self):
        self._pqapi = PQStreamingAPI("logbroker.yandex.net", 2135)
        f = self._pqapi.start()
        f.result(self._TIMEOUT_SECONDS)

        source_id = str(uuid.uuid4())
        simple_writer_config = ProducerConfigurator(topic="yacloud-cloudai--yacloud-cloudai-billing", source_id=source_id)
        self._writer = self._pqapi.create_retrying_producer(simple_writer_config)
        response = self._writer.start_future.result(timeout=self._TIMEOUT_SECONDS)
        assert response.HasField('Init')
        self._current_seq_no = response.Init.MaxSeqNo

    @staticmethod
    def on_write_done_callback(future):
        response = future.result()
        if response.HasField('Error'):
            print "Error"
        elif response.HasField('Ack'):
            # print "OK"
            pass

    def write(self, message):

        # multithread sync
        _csn = 0
        with self._lock:
            self._current_seq_no += 1
            _csn = self._current_seq_no
            self._in_flight_writes += 1
            f = self._writer.write(_csn, message)
        # f.add_done_callback(YaCloudBillingClient.on_write_done_callback)
        # write_result = f.result(timeout=self._TIMEOUT_SECONDS)
        # print(self._current_seq_no)

    def __del__(self):
        if self._writer != None:
            self._writer.stop()
        if self._pqapi != None:
            self._pqapi.stop()

def provide_yacloudbilling_client():
    billing_client = YaCloudBillingClient()
    return billing_client

s = provide_yacloudbilling_client()

class MainHandler(tornado.web.RequestHandler):
    def get(self):
        token = self.request.headers.get("X-YaCloud-SubjectToken")
        projectId = self.request.headers.get("X-YaCloud-ProjectId")
        print token, projectId
        sttt = ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(20))
        s.write(sttt)

class AuthHandler(tornado.web.RequestHandler):
    def get(self):
        token = self.request.headers.get("X-YaCloud-SubjectToken")
        projectId = self.request.headers.get("ProjectId")
        print token, projectId

class StopHandler(tornado.web.RequestHandler):
    def get(self):
        tornado.ioloop.IOLoop.instance().stop()


def make_app():
    return tornado.web.Application([
            (r"/test", MainHandler),
            (r"/auth", AuthHandler),
            (r"/stop", StopHandler),
        ])

def ttt():
    s1 = YaCloudBillingClient()
    s2 = YaCloudBillingClient()
    print s1
    print s2
    s1.__del__()

if __name__ == "__main__":
    app = make_app()
    app.listen(8090)
    tornado.ioloop.IOLoop.current().start()

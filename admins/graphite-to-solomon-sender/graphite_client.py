import logging
import time
from logging.handlers import RotatingFileHandler

from tornado import gen
from tornado.ioloop import IOLoop
from tornado.tcpclient import TCPClient

log = logging.getLogger('graphite_client')


def create_rotating_logging(file, level, max_bytes, backup_count):
    log.setLevel(level)
    fh = RotatingFileHandler(file, maxBytes=max_bytes, backupCount=backup_count)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    fh.setFormatter(formatter)
    log.addHandler(fh)
    log.info("Log Configured")


@gen.coroutine
def setup():
    log.info("Before connect")
    for iteration in range(0, 1000):
        try:
            t1 = time.time()
            client = TCPClient()
            stream = yield client.connect("localhost", 42000)
            log.info("Connected")
            for i in range(0, 10):
                log.info("Before Push Data")
                yield stream.write(bytearray("serg2.metric{} {} {}\n".format(i, iteration, time.time()), "ASCII"))
                log.info("Data Pushed {}".format(i))
            stream.close()
            log.info("Iteration Finished {}".format(iteration))
            t2 = time.time()
            time_to_sleep = 2 - (t2 - t1)
            if time_to_sleep > 0:
                yield gen.sleep(time_to_sleep)
        except Exception as e:
            log.exception("Fail sending to graphite. {}".format(e))


if __name__ == '__main__':
    create_rotating_logging("__grpahite_client.log", "DEBUG", 10000000, 1)
    IOLoop().current().run_sync(setup, timeout=None)

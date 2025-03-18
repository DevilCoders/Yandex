import contextlib
import threading
import time


class RequestSender:
    def __init__(self, suite, n_threads, delay, request_gen):
        self.stop_event = threading.Event()
        self.suite = suite
        self.gen = request_gen
        self.delay = delay
        self.threads = [None] * n_threads

    def start(self):
        for i in range(len(self.threads)):
            self.threads[i] = threading.Thread(target=self)
            self.threads[i].start()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, type, value, traceback):
        self.stop()

    def __call__(self):
        for request in self.gen():
            if self.stop_event.is_set():
                break

            self.suite.send_request(request)
            time.sleep(self.delay)

    def stop(self):
        self.stop_event.set()
        for t in self.threads:
            t.join()

    @contextlib.contextmanager
    def pause(self):
        self.stop()

        try:
            yield
        finally:
            self.start()

import threading
import sys

from cloud.mdb.dbaas_worker.internal.except_hook import set_threading_except_hook


def test_except_hook_capture_exceptions_in_threads():
    expected_exc = RuntimeError("Test exception that raised in bad thread")
    captured_exc = [None]

    def except_hook(_, exc_value, *args, **kwargs):
        captured_exc[0] = exc_value

    sys.excepthook = except_hook
    set_threading_except_hook()

    class BadThread(threading.Thread):
        def run(self):
            raise expected_exc

    bad_thread = BadThread()
    bad_thread.start()
    bad_thread.join()

    assert captured_exc[0] is expected_exc

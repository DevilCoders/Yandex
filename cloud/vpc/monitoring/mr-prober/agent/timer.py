import time


class Timer:
    def __init__(self):
        self._start_time_monotonic = None
        self.restart()

    def restart(self):
        self._start_time_monotonic = time.monotonic()

    def get_total_seconds(self) -> float:
        return time.monotonic() - self._start_time_monotonic

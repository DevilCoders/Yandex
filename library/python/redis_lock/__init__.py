class RedisLock:
    def __init__(self, name, cache, exception_class, timeout=60 * 5, blocking_timeout=0):
        """
        Если blocking_timeout = 0 -> не ждет лок и падает с ошибкой
        Если blocking_timeout > 0 -> будет ждать пока освободится
        """

        self.lock = cache.lock(name, timeout=timeout, blocking_timeout=blocking_timeout)
        self.exception_class = exception_class

    def __enter__(self):
        if self.lock.acquire(blocking=True):
            return self.lock
        raise self.exception_class()

    def __exit__(self, exc_type, exc_value, traceback):
        if self.lock.locked():  # на случай если время выполнения > lock.timeout
            self.lock.release()

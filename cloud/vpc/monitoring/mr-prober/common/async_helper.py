from concurrent.futures import ThreadPoolExecutor


__all__ = ["AsyncHelper"]


class AsyncHelper:
    """
    Helper class for extracting some class's functionality to an external thread.
    Example:
        class Foo:
            def work():
                print("Some heavy work...")

        class AsyncFoo(AsyncHelper, Foo):
            def __init__():
                super().__init()
                self.extract_method_to_thread("work")

        obj = AsyncFoo()
        obj.work() # this method will be run in a separate thread.
        ...
        obj.shutdown_thread() # if we no longer need the thread

        or

        with AsyncFoo() as obj:
            obj.work()

    How it works? This class runs a separate thread in the constructor, after that you can
    override specific methods. Calling these methods make real methods invoked in this thread.
    """
    def __init__(self, *args, **kwargs):
        thread_pool_workers = kwargs.pop("thread_pool_workers", 1)
        thread_name_prefix = kwargs.pop("thread_pool_prefix", self.__class__.__name__)

        super(AsyncHelper, self).__init__(*args, **kwargs)

        # Start a separate thread pool which will do all work
        self._pool = ThreadPoolExecutor(max_workers=thread_pool_workers, thread_name_prefix=thread_name_prefix)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.shutdown_thread()

    # By default, all methods are running in the main thread. Call self.extract_method_to_thread("method")
    # for extracting calls to a separate thread.
    def extract_method_to_thread(self, method_name: str):
        def function(*args, **kwargs):
            return self._pool.submit(getattr(super(AsyncHelper, self), method_name), *args, **kwargs)
        setattr(self, method_name, function)

    def shutdown_thread(self):
        self._pool.shutdown()

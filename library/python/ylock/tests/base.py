import unittest
import pickle
import functools
import time
import six
from multiprocessing import Process, Lock, Value

from ylock import locked, create_manager, decorators


class BaseTestCase(unittest.TestCase):
    timeout = 3600
    prefix = 'ylock-test'

    def setUp(self):
        self.manager = create_manager(self.backend, hosts=self.hosts,
                                      prefix=self.prefix)

    def test_full_name(self):
        self.assertEqual(self.manager.get_full_name('foo'), '%s:foo' % self.prefix)
        self.assertEqual(self.manager.get_full_name(['foo', 'bar']), '%s:foo:bar' % self.prefix)

    def test_lock(self):
        lock = self.manager.lock('foobar', self.timeout)

        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock.release(), True)

    def test_relock(self):
        lock = self.manager.lock('foobar', self.timeout)
        lock1 = self.manager.lock(['foobar'], self.timeout, block=False)

        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock1.acquire(), False)

        self.assertEqual(lock.release(), True)

    def test_check_acquired(self):
        lock = self.manager.lock('foobar', self.timeout)
        lock1 = self.manager.lock('foobar', self.timeout, block=False)

        self.assertEqual(lock1.check_acquired(), False)

        self.assertEqual(lock.acquire(), True)
        self.assertEqual(lock1.check_acquired(), True)

        self.assertEqual(lock.release(), True)
        self.assertEqual(lock1.check_acquired(), False)

    def test_legacy_context_manager(self):
        with locked('test_legacy_context_manager',
                    self.timeout,
                    hosts=self.hosts,
                    backend=self.backend,
                    token=getattr(self, 'token', None),
                    proxy=getattr(self, 'proxy', None)) as is_locked:
            self.assertEqual(is_locked, True)

    def take_lock(self, l, tl):
        with locked('test_lock',
                    timeout=self.timeout,
                    block=False,
                    hosts=self.hosts,
                    backend=self.backend,
                    prefix=self.prefix,
                    token=getattr(self, 'token', None),
                    proxy=getattr(self, 'proxy', None)) as is_locked:
            if is_locked:
                l.acquire()
                tl.value += 1
                l.release()
                time.sleep(3)

    def test_legacy_context_manager_concurrency(self):
        taken_lock = Value('d', 0)
        processes = []
        lock = Lock()

        for i in six.moves.range(10):
            process = Process(target=self.take_lock, args=(lock, taken_lock))
            processes.append(process)
            process.start()

        for process in processes:
            process.join()

        self.assertEqual(taken_lock.value, 1)

    def test_context(self):
        lock = self.manager.lock('lock_context', block=False, timeout=self.timeout)

        self.assertEqual(lock.acquire(), True)

        try:
            try:
                context = lock.get_context()
            except NotImplementedError:
                raise unittest.SkipTest('Context is not supported by backend: %s' % self.backend)

            if context is None:
                return

            pickled_context = pickle.dumps(context, pickle.HIGHEST_PROTOCOL)
            unpicked_context = pickle.loads(pickled_context)

            lock = self.manager.lock_from_context(unpicked_context)
        finally:
            self.assertEqual(lock.release(), True)

    def test_decorator(self):
        val = []

        locked = functools.partial(decorators.locked, manager=self.manager, block=False)

        @locked('test-lock')
        def func():
            val.append(1)

            func()

        func()

        self.assertEqual(val, [1])

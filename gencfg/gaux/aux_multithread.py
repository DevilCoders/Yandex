import multiprocessing
import threading
import time
import copy
import os
import signal
import Queue

import api.cqueue
from kernel.util.errors import formatException

from gaux.aux_utils import print_progress


class WorkingThread(threading.Thread):
    def __init__(self, tid, manager):
        threading.Thread.__init__(self)
        self.tid = tid
        self.manager = manager

    def run(self):
        while True:
            try:
                obj = self.manager.objects.pop()
            except IndexError:
                return

            try:
                result = self.manager.func(obj, self.manager.common_params)
            except Exception, e:
                self.manager.result.append((obj, None, e))
            else:
                self.manager.result.append((obj, result, None))


class MultiThreadRunner(object):
    def __init__(self):
        self.objects = []
        self.result = []

    def run(self, func, objects, common_params, workers, show_progress):
        del workers
        self.func = func
        self.objects = copy.copy(objects)
        self.common_params = common_params
        self.result = []

        total_objects = len(objects)

        workers = map(lambda x: WorkingThread(x, self), range(len(objects)))
        for worker in workers:
            worker.start()

        while len(self.objects):
            time.sleep(1.)
            if show_progress:
                print_progress(len(self.result), total_objects)

        for worker in workers:
            worker.join()

        return self.result


def run_in_multithread(func, objects, common_params, workers=10, show_progress=False):
    mrunner = MultiThreadRunner()
    return mrunner.run(func, objects, common_params, workers, show_progress)


def _wrapper(func, q_in, q_out):
    while True:
        i, x = q_in.get()
        if i is None:
            break
        obj, common_params = x

        try:
            result = func(obj, common_params)
        except Exception, e:
            q_out.put((i, (obj, None, e)))
        else:
            q_out.put((i, (obj, result, None)))


def run_in_multiprocess(func, objects, common_params, workers=10, show_progress=False,
                        finish_on_keyboard_interrupt=False):
    q_in = multiprocessing.Queue(1)
    q_out = multiprocessing.Queue()

    if len(objects) < workers:
        workers = len(objects)

    proc = [multiprocessing.Process(target=_wrapper, args=(func, q_in, q_out)) for _ in range(workers)]
    for p in proc:
        p.daemon = True
        p.start()

    sent = []
    res = []
    try:
        for i, obj in enumerate(objects):
            sent.append(q_in.put((i, (obj, common_params))))
            if show_progress:
                print_progress(i + 1, len(objects))
        [q_in.put((None, None)) for _ in range(workers)]  # to stop worker

        res = [q_out.get() for _ in range(len(sent))]

        [p.join() for p in proc]
    except KeyboardInterrupt:
        if finish_on_keyboard_interrupt:
            for p in proc:
                os.kill(p.pid, signal.SIGKILL)
            if show_progress:
                print_progress(i + 1, len(objects), stopped=True)
            while True:
                try:
                    res.append(q_out.get_nowait())
                except Queue.Empty:
                    break
        else:
            raise

    if show_progress:
        print_progress(len(objects), len(objects), stopped=True)
    return [x for i, x in sorted(res)]


class SkyExecutor(object):
    def __init__(self, func, common_params):
        self.func = func
        self.common_params = common_params
        self.osUser = "skynet"

    def run(self):
        import socket

        try:
            myname = socket.gethostbyaddr(socket.gethostbyname(socket.gethostname()))[0]
        except:
            myname = socket.gethostname()

        result = self.func(myname, self.common_params)

        return result


def run_in_sky(func, hosts, common_params, show_progress=False, finish_on_keyboard_interrupt=False):
    del finish_on_keyboard_interrupt
    client = api.cqueue.Client('cqudp', netlibus=True)

    timeout = common_params.get('timeout', 15) + len(hosts) / 1000

    result = {}
    for host, host_result, failure in client.run(hosts, SkyExecutor(func, common_params)).wait(timeout):
        if failure is not None:
            result[host] = (host, host_result, formatException(failure))
        else:
            result[host] = (host, host_result, None)

        if show_progress:
            print_progress(len(result.keys()), len(hosts))

    if show_progress:
        print_progress(len(result.keys()), len(hosts), stopped=True)

    return map(lambda x: result.get(x, (x, None, Exception("Timed out"))), hosts)


class ERunInMulti(object):
    THREAD = 'THREAD'
    PROCESS = 'PROCESS'
    SKYNET = 'SKYNET'
    ALL = [THREAD, PROCESS, SKYNET]


"""
    General function to run something in multithread/process
    Returns:
        list of triplets <obj, func_result, exception> (func_result is None if timeout, exception is None if no exception)
"""


def run_in_multi(func, objects, common_params, workers=10, mode=ERunInMulti.PROCESS, show_progress=False,
                 finish_on_keyboard_interrupt=False):
    if mode == ERunInMulti.SKYNET:
        return run_in_sky(func, objects, common_params, show_progress)
    elif mode == ERunInMulti.PROCESS:
        return run_in_multiprocess(func, objects, common_params, workers, show_progress, finish_on_keyboard_interrupt)
    elif mode == ERunInMulti.THREAD:
        return run_in_multithread(func, objects, common_params, workers, show_progress)
    else:
        raise Exception("Unknown mode %s" % mode)

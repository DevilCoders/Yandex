"""Test execution context"""

import pytest

import asyncio
import threading

from yc_common.context import get_context, update_context
from yc_common.misc import Thread, parallelize_calls


def test_context_basics():
    assert get_context().__dict__ == {}

    with update_context(project_id="some-project-id", request_id="some-request-id") as context:
        _check_context(context, dict(project_id="some-project-id", request_id="some-request-id"))

        with update_context(request_id="some-other-request-id", instance_id="some-instance-id") as context:
            _check_context(context, dict(project_id="some-project-id", request_id="some-other-request-id",
                                         instance_id="some-instance-id"))

        _check_context(context, dict(project_id="some-project-id", request_id="some-request-id"))

        assert "project_id" in context
        assert context["project_id"] == "some-project-id"

        assert "invalid-key" not in context
        with pytest.raises(KeyError):
            context["invalid-key"]

    assert get_context().__dict__ == {}


def test_std_thread_context():
    def thread_target():
        _check_context(get_context(), {})

        with update_context(thread=2) as context:
            _check_context(context, dict(thread=2))

        _check_context(get_context(), {})

    _check_context(get_context(), {})

    with update_context(thread=1) as context:
        _check_context(context, dict(thread=1))
        thread = threading.Thread()
        thread.start()
        thread.join()
        _check_context(context, dict(thread=1))

    _check_context(get_context(), {})


def test_thread_context():
    thread1_ready = threading.Event()
    thread2_ready = threading.Event()
    parent_ready = threading.Event()

    errors = []

    def save_error(func):
        def wrapper():
            try:
                func()
            except BaseException as e:
                errors.append(e)

        return wrapper

    @save_error
    def thread1_target():
        _check_context(get_context(), dict(parent=True))

        with update_context(thread=1) as context:
            _check_context(context, dict(parent=True, thread=1))
            thread1_ready.set()
            assert thread2_ready.wait(timeout=1)
            _check_context(context, dict(parent=True, thread=1))
            assert parent_ready.wait(timeout=1)

        _check_context(get_context(), dict(parent=True))

    @save_error
    def thread2_target():
        assert thread1_ready.wait(timeout=1)
        _check_context(get_context(), dict(parent=True))

        with update_context(thread=2) as context:
            _check_context(context, dict(parent=True, thread=2))
            thread2_ready.set()
            assert parent_ready.wait(timeout=1)
            _check_context(context, dict(parent=True, thread=2))

        _check_context(get_context(), dict(parent=True))

    threads = []
    _check_context(get_context(), {})

    try:
        with update_context(parent=True) as context:
            threads += [
                Thread(target=thread1_target, derive_context=True),
                Thread(target=thread2_target, derive_context=True),
            ]

            for thread in threads:
                thread.start()

            assert thread2_ready.wait(timeout=1)
            _check_context(context, dict(parent=True))

        _check_context(get_context(), {})
        parent_ready.set()
    finally:
        for thread in threads:
            if thread.is_alive():
                thread.join()

        for error in errors:
            raise error

    _check_context(get_context(), {})


def test_asyncio_context(request):
    loop = asyncio.new_event_loop()

    asyncio.set_event_loop(loop)
    request.addfinalizer(lambda: asyncio.set_event_loop(None))

    task1_started_future = asyncio.Future(loop=loop)
    task2_started_future = asyncio.Future(loop=loop)
    task1_completed_future = asyncio.Future(loop=loop)

    async def task1():
        _check_context(get_context(), {})

        with update_context(task=1) as context:
            _check_context(context, dict(task=1))
            task1_started_future.set_result(None)
            await task2_started_future
            _check_context(context, dict(task=1))

        _check_context(get_context(), {})
        task1_completed_future.set_result(None)

    async def task2():
        await task1_started_future
        _check_context(get_context(), {})

        with update_context(task=2):
            _check_context(get_context(), dict(task=2))
            task2_started_future.set_result(None)
            await task1_completed_future
            _check_context(get_context(), dict(task=2))

        _check_context(get_context(), {})

    completed, timed_out = loop.run_until_complete(asyncio.wait((task1(), task2()), timeout=1))
    assert not timed_out


def test_threaded_asyncio_context():
    async def task1(thread_id, start_processing_event, thread_started_event, thread_wait_event,
                    task1_started_future, task2_started_future, task1_completed_future):
        start_processing_event.wait()
        _check_context(get_context(), {})

        with update_context(thread=thread_id, task=1) as context:
            _check_context(context, dict(thread=thread_id, task=1))

            thread_started_event.set()
            thread_wait_event.wait()
            _check_context(context, dict(thread=thread_id, task=1))

            task1_started_future.set_result(None)
            await task2_started_future
            _check_context(context, dict(thread=thread_id, task=1))

        _check_context(get_context(), {})
        task1_completed_future.set_result(None)

    async def task2(thread_id, task1_started_future, task2_started_future, task1_completed_future):
        await task1_started_future
        _check_context(get_context(), {})

        with update_context(thread=thread_id, task=2):
            _check_context(get_context(), dict(thread=thread_id, task=2))
            task2_started_future.set_result(None)
            await task1_completed_future
            _check_context(get_context(), dict(thread=thread_id, task=2))

        _check_context(get_context(), {})

    def thread_target(thread_id, start_processing_event, thread_started_event, thread_wait_event):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)

        task1_started_future = asyncio.Future(loop=loop)
        task2_started_future = asyncio.Future(loop=loop)
        task1_completed_future = asyncio.Future(loop=loop)

        completed, timed_out = loop.run_until_complete(asyncio.wait((
            task1(thread_id, start_processing_event, thread_started_event, thread_wait_event,
                  task1_started_future, task2_started_future, task1_completed_future),
            task2(thread_id, task1_started_future, task2_started_future, task1_completed_future),
        ), timeout=1))
        assert not timed_out

    start_processing_event = threading.Event()
    thread1_started_event = threading.Event()
    thread2_started_event = threading.Event()

    start_processing_event.set()
    parallelize_calls(thread_target, [
        (1, start_processing_event, thread1_started_event, thread2_started_event),
        (2, thread1_started_event, thread2_started_event, start_processing_event),
    ])


def _check_context(context, expected_contents):
    assert context.__dict__ == expected_contents
    assert get_context() is context

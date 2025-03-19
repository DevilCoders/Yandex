"""Execution context preserving tools.

Attention: Be very careful in passing context object between functions: with current implementation each task/thread has
           the same context object which magically returns different data depending on the task/thread it's accessed
           from.
"""

import asyncio
import functools
import threading

import tasklocals


class _ContextBase:
    def __contains__(self, item):
        return item in self.__dict__

    def __getitem__(self, item):
        return self.__dict__[item]

    def get(self, key, default=None):
        return self.__dict__.get(key, default)

    def to_dict(self):
        return self.__dict__.copy()


class _ThreadContext(threading.local, _ContextBase):
    pass


class _TaskContext(tasklocals.local, _ContextBase):
    def __new__(cls, *args, **kwargs):
        self = tasklocals.local.__new__(cls, *args, **kwargs)

        try:
            impl = object.__getattribute__(self, "_local__impl")
            _ = impl._loop
        except AttributeError:
            raise Exception("Unable to access tasklocals internals")

        # tasklocals is designed to be attached to only one loop which is determined at the time of the creation of the
        # local object, which is not suitable for us. So force the loop to be determined in runtime.
        impl._loop = None

        return self


_THREAD_CONTEXT = _ThreadContext()
_THREAD_LOCALS = threading.local()


def get_context():
    # asyncio.get_event_loop() creates a new loop if it's not set yet, so check its existence manually
    loop = asyncio.get_event_loop_policy()._local._loop

    if loop is not None and asyncio.current_task(loop=loop) is not None:
        # If we're inside a loop and processing some asynchronous task, return task-local context

        try:
            return _THREAD_LOCALS.task_context
        except AttributeError:
            task_context = _THREAD_LOCALS.task_context = _TaskContext()
            return task_context
    else:
        # For synchronous loop tasks and code outside of a loop return thread-local context
        return _THREAD_CONTEXT


def context_session(**kwargs):
    # clear session context after exit from context manager
    class Session:
        def __enter__(self):
            return context

        def __exit__(self, exc_type, exc_val, exc_tb):
            context_dict.clear()

    context = get_context()

    context_dict = context.__dict__
    context_dict.clear()
    context_dict.update(kwargs)

    return Session()


def switch_context(**kwargs):
    # restore old session context after exit from context manager
    class Session:
        def __enter__(self):
            return context

        def __exit__(self, exc_type, exc_val, exc_tb):
            context_dict.clear()
            context_dict.update(old_context_dict)

    context = get_context()

    context_dict = context.__dict__
    old_context_dict = context_dict.copy()
    context_dict.clear()
    context_dict.update(kwargs)

    return Session()


def update_context(**kwargs):
    class Session:
        def __enter__(self):
            return context

        def __exit__(self, exc_type, exc_val, exc_tb):
            for key, value in to_set:
                context_dict[key] = value

            for key in to_delete:
                context_dict.pop(key, None)

    to_set = []
    to_delete = []

    context = get_context()
    context_dict = context.__dict__

    for key, value in kwargs.items():
        if key in context_dict:
            to_set.append((key, context_dict[key]))
        else:
            to_delete.append(key)

        context_dict[key] = value

    return Session()


def apply_context(callable):
    context_dict = get_context().to_dict()

    @functools.wraps(callable)
    def wrapper(*args, **kwargs):
        with update_context(**context_dict):
            return callable(*args, **kwargs)

    return wrapper

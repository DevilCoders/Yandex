import Queue
import logging
from collections import defaultdict
from enum import IntEnum


class EventType(IntEnum):
    SOLOMON_METRICS = 0
    APPEND_REQUEST = 1


class Pipeline:

    def __init__(self, queues):
        self.queues = queues
        self.listeners = defaultdict(list)

    def put(self, event_type, message):
        self.queues[0].put_nowait((event_type, message))

    def register_event_listener(self, event_type, event_listener):
        self.listeners[event_type].append(event_listener)

    def run_listener_iteration(self):
        for queue in self.queues:
            while not queue.empty():
                try:
                    event_type, message = queue.get_nowait()
                    for listener in self.listeners.get(event_type):
                        listener(message)
                except Queue.Empty:
                    break
                except Exception:  # may happen if child process is terminated with kill
                    logging.exception("error while reading from pipeline")
                    pass

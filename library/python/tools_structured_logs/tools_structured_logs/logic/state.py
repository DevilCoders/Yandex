# coding: utf-8

from threading import local

from collections import defaultdict


class VendorInfo(object):
    def __init__(self):
        self._milliseconds = 0
        self._called_times = 0

    def increment(self, milliseconds):
        self._milliseconds += milliseconds
        self._called_times += 1

    def update_both(self, msec, times):
        self._milliseconds += msec
        self._called_times += times


class StateItem(object):
    _vendors = None

    def __init__(self):
        self._vendors = defaultdict(VendorInfo)

    def add_time(self, vendor, milliseconds):
        self._vendors[vendor].increment(milliseconds)

    def __repr__(self):
        return ', '.join((
            '{vendor}: {msec}msec, {times}'.format(
                vendor=key,
                msec=self._vendors[key]._milliseconds,
                times=self._vendors[key]._called_times,
            )
            for key in self._vendors
        ))

    def append_info(self, some_item):
        """

        @type some_item: StateItem
        """
        for vendor, called in some_item._vendors.items():
            self._vendors[vendor].update_both(
                msec=some_item._vendors[vendor]._milliseconds,
                times=some_item._vendors[vendor]._called_times,
            )

    def items(self):
        return self._vendors.items()


class ThreadLocalState(local):
    def __init__(self):
        self.items = []

    def reset(self):
        self.items = []

    def add_time(self, vendor, time):
        last_item = self.get_last_item()
        if last_item:
            last_item.add_time(vendor, time)

    def put_state(self):
        self.items.append(StateItem())

    def pop_state(self):
        last_item = self.items.pop()
        new_last_item = self.get_last_item()
        if new_last_item:
            new_last_item.append_info(last_item)
        return last_item

    def get_last_item(self):
        if self.items:
            return self.items[-1]

    def is_enabled(self):
        return bool(self.items)


_state = ThreadLocalState()


def get_state():
    return _state


def reset_state(sender, **kwargs):
    state = get_state()
    state.reset()

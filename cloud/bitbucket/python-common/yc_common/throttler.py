"""A generic throttling class."""

import time

from typing import Tuple, Iterable


class Throttler:
    def __init__(self, limit, period):
        self.limit = limit
        self.period = period
        self.__slots = {}

    def acquire(self, key):
        cur_time = time.monotonic()
        slot = self.__slots.get(key)

        if slot is None:
            slot = self.__slots[key] = _Slot(cur_time, self.period, self.limit)
        elif cur_time >= slot.expire_time:
            slot.reset(cur_time, self.period, self.limit)

        return slot.acquire()

    def iter_rejects(self, only_for_expired_slots=False) -> Iterable[Tuple[str, int]]:
        cur_time = time.monotonic()

        for key, slot in self.__slots.items():
            if slot.rejected and (not only_for_expired_slots or cur_time >= slot.expire_time):
                yield (key, slot.rejected)
                slot.rejected = 0

    def get_closest_rejects_expiration_timeout(self):
        closest_expire_time = None

        for slot in self.__slots.values():
            if slot.rejected:
                if closest_expire_time is None:
                    closest_expire_time = slot.expire_time
                else:
                    closest_expire_time = min(closest_expire_time, slot.expire_time)

        if closest_expire_time is None:
            return

        return max(0, closest_expire_time - time.monotonic())

    def reset(self):
        self.__slots.clear()


class _Slot:
    __slots__ = ["remain", "rejected", "expire_time"]

    def __init__(self, cur_time, period, limit):
        self.rejected = 0
        self.reset(cur_time, period, limit)

    def acquire(self):
        if self.remain > 0:
            self.remain -= 1
            rejected, self.rejected = self.rejected, 0
            return True, rejected
        else:
            self.rejected += 1
            return False, self.rejected

    def reset(self, cur_time, period, limit):
        self.remain = limit
        self.expire_time = cur_time + period

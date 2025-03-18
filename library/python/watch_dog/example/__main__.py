import time

import library.python.watch_dog as lpw


@lpw.with_watchdog(2)
def func():
    time.sleep(100)


if __name__ == '__main__':
    func()

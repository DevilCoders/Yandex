import os

import library.python.guid as guid


if __name__ == '__main__':
    print(guid.guid())

    pid = os.fork()

    if pid:
        os.waitpid(pid, 0)

    print(guid.guid())

    pid = os.fork()

    if pid:
        os.waitpid(pid, 0)

    print(guid.guid())

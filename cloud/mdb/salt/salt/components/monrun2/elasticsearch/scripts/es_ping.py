#!/usr/bin/env python3.6
import time

from es_lib import parse_args, get_client, die


def main():
    """
    Program entry point.
    """
    args = parse_args()
    error = ''
    for n in range(args.number):
        try:
            if not get_client(args.user, args.password).ping():
                continue

            die(0, 'OK')

        except Exception as e:
            error = repr(e)

        time.sleep(1)

    die(2, f'service is dead {": " + error if error else ""}')


if __name__ == '__main__':
    main()

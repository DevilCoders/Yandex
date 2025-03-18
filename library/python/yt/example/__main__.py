import argparse
import logging
import time

import library.python.yt as lpy
import library.python.init_log as lpi

import yt.wrapper as yw

logger = logging.getLogger('example')


def pause():
    try:
        while True:
            logger.info("Sleeping...")
            time.sleep(10)
    except KeyboardInterrupt:
        logger.info("Sleep interrupted")
        return


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--proxy', default='hahn')
    parser.add_argument('--path', default='//tmp/{user}/lock')
    parser.add_argument('--mode', default='context_manager', choices=['context_manager', 'methods'])
    parser.add_argument('--timeout', type=float)
    parser.add_argument('--verbose', action='store_true')

    args = parser.parse_args()

    lpi.init_log(level='DEBUG' if args.verbose else 'INFO')
    yw.config.set_proxy(args.proxy)

    if args.mode == 'context_manager':
        logger.info("Using lock context manager")
        logger.info("Acquiring lock...")

        with lpy.Lock(args.path, identifier='example'):
            logger.info("Under lock!")
            pause()

        logger.info("Lock released!")

    elif args.mode == 'methods':
        logger.info("Using lock acquire/release methods")
        lock = lpy.Lock(args.path, identifier='example')
        logger.info("Acquiring lock...")

        if lock.acquire(timeout=args.timeout):
            logger.info("Under lock!")
            pause()
            lock.release()
            logger.info("Lock released!")
        else:
            logger.info("Failed to acquire lock within timeout")


if __name__ == "__main__":
    main()

import argparse
import logging
import random
import time

import luigi
import luigi.interface

import library.python.init_log as lpi

logger = logging.getLogger(__name__)

complete = set()
tasks = []


class Task(luigi.Task):
    N = luigi.IntParameter(default=0)
    Deps = luigi.ListParameter(default=[])

    def run(self):
        time.sleep(1)
        complete.add(self.N)

        print(self.N)

    def requires(self):
        return [tasks[x] for x in self.Deps]

    def output(self):
        return self

    def exists(self):
        return self.N in complete


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--scheduler-url', default='http://api-luigi.n.yandex-team.ru')
    parser.add_argument('--task-count', default=100)
    parser.add_argument('--max-deps', default=3)
    parser.add_argument('--verbose', action='store_true')

    args = parser.parse_args()

    # Luigi creates its own log handlers by default, disable this feature
    luigi_config = luigi.configuration.get_config()
    luigi_config.set('core', 'no_configure_logging', 'true')

    lpi.init_log(level='DEBUG' if args.verbose else 'INFO', append_ts=True)

    logger.info('args: %s', args)

    for n in range(0, args.task_count):
        if tasks:
            deps_count = min(random.randint(1, args.max_deps), len(tasks))
            deps = [x.N for x in random.sample(tasks, deps_count)]
        else:
            deps = []

        tasks.append(Task(n, deps))

    luigi.build(tasks, scheduler_url=args.scheduler_url)

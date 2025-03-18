# -*- coding: utf-8 -*-

import random

from core.actions.taskdealer import get_t_testing

DEBUG = False
PRINT_HISTORY = True


class Task:
    # statuses: ([0 - 'U', 1 - 'A', 2 - 'C', 3 - 'R'], age)
    def __init__(self, assessors_count):
        self.statuses = [[0, 0] for i in range(assessors_count)]
        self.inspection = False


class TaskPool:
    def __init__(self, tasks_count, assessors_count):
        self.tasks = [Task(assessors_count) for i in range(tasks_count)]


class Assessor:
    def __init__(self):
        # [flag, age]
        self.rest = [False, 0]
        # {day: (Inspection, Regular+Inspection), ...}
        self.history_take_tasks = {}
        self.history_inspection_solved = {}


class AssessorFactory:
    # probabilities format:
    # [(take, (reject, complete, timeout_instantly),
    #   rest (20 hours long)), ...]
    def __init__(self, assessors_count, probabilities, taskpools_count,
                 tasks_counts, cycle_timeout):
        (self.assessors_count, self.probabilities, self.taskpools_count,
         self.tasks_counts, self.cycle_timeout) =\
            (assessors_count, probabilities, taskpools_count,
             tasks_counts, cycle_timeout)
        self.taskpools = [TaskPool(tasks_counts[i], assessors_count)
                          for i in range(taskpools_count)]
        self.assessors = [Assessor() for i in range(assessors_count)]
        self.task_batch_size = 10

    def run(self, iterations):
        for i in range(iterations):
            if DEBUG:
                print('\t>>hour %d<<' % i)

            for assessor in range(self.assessors_count):
                if self.assessors[assessor].rest[0]:
                    self.assessors[assessor].rest[1] += 1
                    if self.assessors[assessor].rest[1] < 20:
                        continue
                    else:
                        self.assessors[assessor].rest[0] = False

                r = random.random()
                if r < self.probabilities[assessor][2]:
                    # assessor day of rest
                    self.assessors[assessor].rest[0] = True
                    self.assessors[assessor].rest[1] = 0
                    continue

                r = random.random()
                if r < self.probabilities[assessor][0]:
                    # take_tasks chosen
                    self.take_tasks(assessor, i)

                for p in self.taskpools:
                    for t in p.tasks:
                        if t.statuses[assessor][0] == 1:
                            # increment task time
                            t.statuses[assessor][1] += 1
                            if t.statuses[assessor][1] > self.cycle_timeout:
                                t.statuses[assessor][0] = 0
                                continue

                            r = random.random()
                            if r < self.probabilities[assessor][1][0]:
                                # reject task chosen
                                t.statuses[assessor][0] = 3
                            elif r < (self.probabilities[assessor][1][0] +
                                      self.probabilities[assessor][1][1]):
                                # complete task chosen
                                t.statuses[assessor][0] = 2
                                if PRINT_HISTORY:
                                    if t.inspection:
                                        hist = self.assessors[assessor].\
                                            history_inspection_solved
                                        hist[i] =\
                                            hist.get(i, 0) + 1
                            else:
                                # timeout task chosen
                                t.statuses[assessor][0] = 0

    def take_tasks(self, assessor, iteration):
        t_done = sum([len(
            [1 for t in pool.tasks if t.statuses[assessor][0] == 2]
        ) for pool in self.taskpools])

        t_verified = sum([len(
            [1 and t.inspection for t in pool.tasks
             if t.statuses[assessor][0] == 2 and t.inspection]
        ) for pool in self.taskpools])

        t_testing = get_t_testing(t_done, t_verified)

        needed_inspection_task_count = min(self.task_batch_size, t_testing)

        available_inspection_task_count = sum([len(
            [1 for t in pool.tasks
             if t.statuses[assessor][0] == 0 and t.inspection]
        ) for pool in self.taskpools])

        if available_inspection_task_count < needed_inspection_task_count:
            # change regular tasks to inspection
            available_tasks = reduce(lambda x, y: x + y,
                                     [[t for t in pool.tasks
                                       if (t.statuses[assessor][0] != 3 and
                                           not t.inspection)]
                                      for pool in self.taskpools])
            for t in random.sample(available_tasks,
                                   (needed_inspection_task_count -
                                    available_inspection_task_count)):
                t.inspection = True
        # get inspection tasks for user
        inspection_tasks = reduce(lambda x, y: x + y,
                                  [[t for t in pool.tasks
                                    if (t.statuses[assessor][0] == 0 and
                                        t.inspection)]
                                   for pool in self.taskpools])

        inspection_tasks_count = min(needed_inspection_task_count,
                                     len(inspection_tasks))

        for t in random.sample(inspection_tasks,
                               inspection_tasks_count):
            t.statuses[assessor][0] = 1
            t.statuses[assessor][1] = 0

        # get regular+inspection tasks for user
        avail_tasks = reduce(lambda x, y: x + y,
                             [[t for t in pool.tasks
                               if t.statuses[assessor][0] == 0]
                              for pool in self.taskpools])

        for t in random.sample(avail_tasks,
                               min(self.task_batch_size -
                                   inspection_tasks_count,
                                   len(avail_tasks))):
            t.statuses[assessor][0] = 1
            t.statuses[assessor][1] = 0

        if DEBUG:
            print('Assessor%d took %d I, %d R+I' % (
                assessor, inspection_tasks_count,
                min(self.task_batch_size - inspection_tasks_count,
                    len(avail_tasks))))
        if PRINT_HISTORY:
            self.assessors[assessor].history_take_tasks[iteration] = (
                inspection_tasks_count,
                min(self.task_batch_size - inspection_tasks_count,
                    len(avail_tasks))
            )

    def get_assessor_stat(self, assessor):
        t_done = sum([len(
            [1 for t in pool.tasks if t.statuses[assessor][0] == 2]
        ) for pool in self.taskpools])

        t_rejected = sum([len(
            [1 for t in pool.tasks if t.statuses[assessor][0] == 3]
        ) for pool in self.taskpools])

        t_verified = sum([len(
            [1 and t.inspection for t in pool.tasks
             if t.statuses[assessor][0] == 2 and t.inspection]
        ) for pool in self.taskpools])

        t_testing = get_t_testing(t_done, t_verified)

        return 'done: %d; rejected: %d; verified: %d; uninspected: %d' % (
            t_done, t_rejected, t_verified, t_testing)

    def get_inspection_count(self):
        return sum([len(
            [1 for t in pool.tasks if t.inspection]
        ) for pool in self.taskpools])

    def get_assessor_history_take_tasks(self, assessor):
        return '\n'.join([
            '%d: ' % key +
            '%dI + %d(R+I)' % self.assessors[assessor].history_take_tasks[key]
            for key in sorted(
                self.assessors[assessor].history_take_tasks.iterkeys())])

    def get_assessor_history_inspection_tasks(self, assessor, aggr_period):
        hist = self.assessors[assessor].history_inspection_solved
        res = []
        for key in sorted(hist.iterkeys()):
            while key >= aggr_period * len(res):
                res.append(0)
            res[-1] += hist[key]
        return '\n'.join([
            '%d-%d: %d' % (i * aggr_period, (i + 1) * aggr_period, res[i])
            for i in range(len(res))])


if __name__ == '__main__':
    assessors_count = 10
    # [(take, (reject, complete, timeout_instantly),
    #   rest (20 hours long)), ...]
    probabilities = [
        (0.15 + random.random() * 0.1,
         (0.04 + random.random() * 0.02,
          0.7 + random.random() * 0.1,
          0.01 + random.random() * 0.01),
         2.0 / 7 - random.random() * 0.05)
        for assessor in range(assessors_count)
    ]
    taskpools = 1
    tasks_counts = (1000,)
    cycle_timeout = 20
    iterations = 2000

    af = AssessorFactory(
        assessors_count, probabilities, taskpools, tasks_counts, cycle_timeout
    )
    af.run(iterations)

    print(
        'STAT:\n' +
        '\n'.join(['\tAssessor%d: %s' % (i, af.get_assessor_stat(i))
                   for i in range(assessors_count)]) +
        '\n\t\tTasks for assessor administrator: %d' %
        af.get_inspection_count()
    )

    if PRINT_HISTORY:
        print(
            '\nHISTORY:\n' +
            '\n'.join(['\tAssessor%d:\n%s' % (
                i, af.get_assessor_history_take_tasks(i))
                for i in range(assessors_count)])
        )

    if PRINT_HISTORY:
        print(
            '\nINSPECTION TASKS:\n' +
            '\n'.join(['\tAssessor%d:\n%s' % (
                i, af.get_assessor_history_inspection_tasks(i, 100))
                for i in range(assessors_count)])
        )

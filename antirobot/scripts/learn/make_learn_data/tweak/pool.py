import multiprocessing
from collections import deque
import time

# Run a set of tasks simultaneuosly
# Each task jas weight
# The sum of all running task cannot be larger than maxWeight
#
# Usage: 
#   pool = Scheduler(maxWeight)
#   pool.Add(weight1, Func1, arg1, arg2..)
#   pool.Add(weightN, FuncN, ...)
#   pool.Run()
class Scheduler:
    @staticmethod
    def Launcher(func, *args, **kw):
        def Func():
            return func(*args, **kw)

        return Func

    class Task:
        def __init__(self, weight, func):
            self.weight = int(weight)
            self.func = func
            self.process = None
            self.index = None

        def __cmp__(self, other):
            return self.weight - other.weight

        def IsAlive(self):
            return self.process and self.process.is_alive()

        def HasFinished(self):
            return self.process and not self.process.is_alive()

        def RunTask(self):
            self.process = multiprocessing.Process(target = self.func)
            self.process.start()

        def Wait(self):
            while self.IsAlive():
                time.sleep(1)

        def Clear(self):
            self.process = None

    def __init__(self, maxWeight):
        self.maxWeight = maxWeight
        self.taskList = []
        self.queue = deque()
        self.failedCount = 0
        self.workingCount = 0

    def Add(self, weight, func, *args, **kw):
        if self.maxWeight and weight > self.maxWeight:
           raise Exception, "Weight of task is greater than max weight of this pool execution"

        task = self.Task(weight, self.Launcher(func, *args, **kw))
        task.index = len(self.taskList)
        self.taskList.append(task)

    def PrepareTasks(self):
        self.taskList.sort()

        for t in self.taskList:
            self.queue.append(t)

    def Run(self):
        def GetRunningTasks():
            res = []
            for t in self.taskList:
                if t.IsAlive():
                    res.append("%d" % t.index)

            return ",".join(res)

        def GetQueueTasks():
            res = []
            for i, t in enumerate(self.queue):
                res.append("%d" % t.index)

            return ",".join(res)

        self.PrepareTasks()
        curWeight = 0
        while True:
            if len(self.queue) == 0:
                break

            if self.maxWeight == 0 and curWeight == 0:
                task = self.queue.popleft()
                task.RunTask()
                curWeight += task.weight
                continue

            if (curWeight + self.queue[0].weight) <= self.maxWeight:
                task = self.queue.popleft()
                task.RunTask()
                curWeight += task.weight
                continue


            finTask = self.__FindFinished()
            if finTask:
                curWeight -= finTask.weight
                if finTask.process.exitcode != 0:
                    self.failedCount += 1
                finTask.Clear()
                continue

            time.sleep(1)

        for task in self.taskList:
            if task.IsAlive():
                task.Wait()
                if task.process.exitcode != 0:
                    self.failedCount += 1
                task.Clear()

    def GetFailedCount(self):
        return self.failedCount

    def __FindFinished(self):
        for i, task in enumerate(self.taskList):
            if task.HasFinished():
                return task

        return None


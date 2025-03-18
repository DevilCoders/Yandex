from .conftest import TaskResult, TestTask, TestGroupPool


def test_sequential():
    pool = TestGroupPool(2)
    task1 = TestTask(1, 0, max_attempts=1, successes_on=1)
    task2 = TestTask(2, 0, max_attempts=1, successes_on=1)

    pool.submit(task1)
    pool.submit(task2)
    pool.wait_idle()
    pool.stop()

    assert task1.done()
    assert task1.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=0)
    assert task2.done()
    assert task2.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=0)


def test_parallel():
    pool = TestGroupPool(2)
    task1 = TestTask(1, 0, max_attempts=1, successes_on=1)
    task2 = TestTask(2, 1, max_attempts=1, successes_on=1)

    pool.submit(task1)
    pool.submit(task2)
    pool.wait_idle()
    pool.stop()

    assert task1.done()
    assert task1.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=0)
    assert task2.done()
    assert task2.result(0) == TaskResult(group_id=1, worker_id=0, worker_incarnation=0)


def test_dependency():
    pool = TestGroupPool(2)
    task1 = TestTask(1, 0, max_attempts=1, successes_on=1)
    task2 = TestTask(2, 1, max_attempts=1, successes_on=1)
    task3 = TestTask(3, 1, max_attempts=1, successes_on=1)
    task4 = TestTask(4, 0, max_attempts=1, successes_on=1)

    pool.submit(task2, task1)
    pool.submit(task3)
    pool.wait_idle()

    assert not task2.done()
    assert task3.done()
    assert task3.result(0) == TaskResult(group_id=1, worker_id=0, worker_incarnation=0)

    pool.submit(task1)
    pool.wait_idle()

    assert task1.done()
    assert task1.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=0)
    assert task2.done()
    assert task2.result(0) == TaskResult(group_id=1, worker_id=0, worker_incarnation=0)

    pool.submit(task4, task1)
    pool.wait_idle()
    pool.stop()

    assert task4.done()
    assert task4.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=0)


def test_dependency_chain_failure():
    pool = TestGroupPool(2)
    task1 = TestTask(1, 0, max_attempts=1, successes_on=0)
    task2 = TestTask(2, 1, max_attempts=1, successes_on=1)

    pool.submit(task1)
    pool.submit(task2, task1)
    pool.wait_idle()
    pool.stop()

    assert task1.done()
    assert task1.exception() is not None
    assert task2.done()
    assert task2.exception() is not None


def test_worker_restart():
    pool = TestGroupPool(2)
    task = TestTask(1, 0, max_attempts=3, successes_on=3)

    pool.submit(task)
    pool.wait_idle()
    pool.stop()

    assert task.result(0) == TaskResult(group_id=0, worker_id=0, worker_incarnation=2)

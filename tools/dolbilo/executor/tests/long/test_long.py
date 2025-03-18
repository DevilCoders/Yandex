import pytest
from yatest.common import process


@pytest.mark.parametrize('requests,limit', (
    (50, '--rps-fixed 10 --queries-limit 50'),
    (51, '--rps-fixed 10 --time-limit 5'),
    (51, '--rps-schedule const(10,5s)'),
))
def test_huge_plan_processing(executor, huge_plan, balancer, limit, requests):
    before = balancer.status()
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', huge_plan, '--replace-host', 'localhost',
    ] + limit.split(), timeout=10)
    after = balancer.status()
    assert after['requests'] - before['requests'] == requests
    assert 5 < proc.elapsed < 7
    assert proc.std_out == '' and proc.std_err == 'Process shots...\ndone\n'


@pytest.mark.parametrize('requests,limit', (
    (50, '--rps-fixed 10 --queries-limit 50'),
    (50, '--rps-fixed 10 --time-limit 5'),  # FIXME: `--circular' changes this `requests'
    (51, '--rps-schedule const(10,5s)'),
))
@pytest.mark.parametrize('memory,memout', (
    ('', ''),
    ('--plan-memory', 'Loading plan...\nDone loading plan (loaded 1 items)\n'),
))
def test_circular_plan_processing(executor, single_ping_plan, balancer, limit, requests, memory, memout):
    before = balancer.status()
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', single_ping_plan, '--replace-host', 'localhost',
        '--circular',
    ] + limit.split() + memory.split(), timeout=10)
    after = balancer.status()
    assert after['requests'] - before['requests'] == requests  # one extra shot in the end
    assert 5 < proc.elapsed < 7
    assert proc.std_out == '' and proc.std_err == 'Process shots...\n' + memout + 'done\n'


# NB: it takes ~5min of mostly IDLE cpu
def test_stpd_barrel(executor, exact_plan, exact_ms, balancer):
    before = balancer.status()
    assert len(balancer.accesslog_us()) == 0  # balancer should not be session-shared
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', exact_plan], timeout=330)
    after = balancer.status()
    alog = balancer.accesslog_us()
    assert proc.std_out == '' and proc.std_err == 'Process shots...\ndone\n'
    assert after['requests'] - before['requests'] == len(exact_ms) == len(alog)
    # Can't tolerate testenv delays well, so only one digit is used
    dalog = [str(round((i - j) / 1e6)) for i, j in zip(alog[1:], alog)]
    dtrue = [str(round((i - j) / 1e3)) for i, j in zip(exact_ms[1:], exact_ms)]
    assert dtrue != ['0.0'] * len(dtrue)
    assert dalog == dtrue

import pytest
from yatest.common import process, network


@pytest.mark.parametrize('replace_host', ('127.0.0.1', '::1', ''))  # default is `localhost`
def test_smoking_barrel(executor, single_ping_plan, balancer, replace_host):
    replace_host = ['--replace-host', replace_host] if replace_host else []
    before = balancer.status()
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', single_ping_plan, '--circular',
        '--rps-schedule', 'const(10, 1s)',
    ] + replace_host, timeout=5)
    after = balancer.status()
    alog = balancer.accesslog_us()
    assert proc.std_out == '' and proc.std_err == 'Process shots...\ndone\n'
    assert after['requests'] - before['requests'] == len(alog) == 11  # one extra shot in the end
    assert 1 < proc.elapsed < 3
    diff = [str(round((i - j) / 1e6, 2)) for i, j in zip(alog[1:], alog)]
    assert diff == ['0.1'] * 10


def test_plan_echo(executor, single_ping_plan):
    proc = process.execute([executor, '--echo', '--plan-file', single_ping_plan])
    assert proc.std_out.startswith('localhost:80/GET /ping HTTP/1.1\r\n')


def test_plan_echo_ammo(executor, single_ping_plan):
    proc = process.execute([executor, '--echo', 'phantom-ammo', '--plan-file', single_ping_plan])
    assert proc.std_out.startswith('58\nGET /ping HTTP/1.1\r\n')


def test_stpd_echo(executor, exact_stpd, exact_plan):
    ex = process.execute([executor, '--echo', 'phantom-stpd', '--plan-file', exact_plan])
    assert ex.std_out == open(exact_stpd).read()


@pytest.mark.xfail
def test_nonexitent_host_failure(executor, single_ping_plan):
    with pytest.raises(process.ExecutionError):
        with network.PortManager() as pm:
            closed_port = pm.get_port()
            _ = process.execute([
                executor, '--replace-port', str(closed_port),
                '--output', '/dev/null',
                '--replace-host', 'nonexistent.yndx.net',
                '--plan-file', single_ping_plan, '--circular',
                '--rps-schedule', 'const(10, 5s)'], timeout=1)


@pytest.mark.xfail
def test_nonexitent_plan_failure(executor):
    with pytest.raises(process.ExecutionError):
        with network.PortManager() as pm:
            closed_port = pm.get_port()
            _ = process.execute([
                executor, '--replace-port', str(closed_port),
                '--output', '/dev/null',
                '--plan-file', '/nonexistent', '--circular',
                '--rps-schedule', 'const(10, 5s)'], timeout=1)


@pytest.mark.xfail
def test_keepalive(executor, single_ping_plan, balancer):
    before = balancer.status()
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', single_ping_plan, '--circular',
        '--rps-schedule', 'const(10, 5s)',
        '--keep-alive'
    ], timeout=10)
    after = balancer.status()
    assert after['requests'] - before['requests'] == 51
    assert after['accepts'] - before['accepts'] < 5
    assert 5 < proc.elapsed < 7


def test_real_keepalive(executor, single_ping_plan, balancer):
    before = balancer.status()
    proc = process.execute([
        executor, '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', single_ping_plan, '--circular',
        '--rps-schedule', 'const(10, 5s)',
        '--real-keep-alive'
    ], timeout=10)
    after = balancer.status()
    assert after['requests'] - before['requests'] == 51
    assert after['accepts'] - before['accepts'] < 5
    assert 5 < proc.elapsed < 7


def test_closed_port_execution(executor, single_ping_plan):
    """ STDERR flood:
    2016-02-16T16:00:20.176336Z (Input/output error) tools/dolbilo/executor/main.cpp:1031: cannot connect(localhost, 127.0.0.1, 12512), url http://localhost/ping  # noqa
    2016-02-16T16:00:20.274985Z (Input/output error) tools/dolbilo/executor/main.cpp:1031: cannot connect(localhost, 127.0.0.1, 12512), url http://localhost/ping  # noqa
    2016-02-16T16:00:20.375667Z (Input/output error) tools/dolbilo/executor/main.cpp:1031: cannot connect(localhost, 127.0.0.1, 12512), url http://localhost/ping  # noqa
    ...
    """
    with network.PortManager() as pm:
        closed_port = pm.get_port()
        proc = process.execute([
            executor, '--replace-port', str(closed_port),
            '--output', '/dev/null',
            '--plan-file', single_ping_plan, '--circular',
            '--rps-schedule', 'const(10, 5s)'
        ], timeout=10)
    assert 5 < proc.elapsed < 7


def test_heatup_case(executor, single_ping_plan, balancer):
    before = balancer.status()
    _ = process.execute([
        executor, '--replace-host', 'localhost', '--replace-port', str(balancer.port),
        '--output', '/dev/null',
        '--plan-file', single_ping_plan, '--circular',
        '--mode', 'finger', '--simultaneous', '6', '--queries-limit', '500',
    ], timeout=10)
    after = balancer.status()
    assert after['requests'] - before['requests'] == 500

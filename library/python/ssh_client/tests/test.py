import os
import sys
import pytest
import subprocess
import library.python.ssh_client as ssh


# https://github.com/python/cpython/blob/master/Lib/test/test_subprocess.py


encoding = {'encoding': 'utf8'} if sys.version_info[0] > 2 else {}


@pytest.fixture(params=[None, 'localhost'])
def hostname(request):
    return request.param


@pytest.fixture
def client(request, hostname):
    return ssh.SshClient(hostname=hostname)


def test_api():
    assert ssh.CalledProcessError == subprocess.CalledProcessError
    assert ssh.TimeoutExpired == subprocess.TimeoutExpired
    assert ssh.CompletedProcess == subprocess.CompletedProcess
    assert ssh.PIPE == subprocess.PIPE
    assert ssh.DEVNULL == subprocess.DEVNULL
    assert ssh.STDOUT == subprocess.STDOUT


def test_run(client):
    res = client.run(['true'])
    assert type(res) is ssh.CompletedProcess
    assert res.returncode == 0
    assert res.stdout is None
    assert res.stderr is None

    res = client.run(['false'])
    assert type(res) is ssh.CompletedProcess
    assert res.returncode == 1
    assert res.stdout is None
    assert res.stderr is None


def test_run_output(client):
    res = client.run(['echo', 'test'], stdout=ssh.PIPE)
    assert res.stdout == b'test\n'
    assert res.stderr is None

    res = client.run(['echo', 'test'], stdout=ssh.PIPE, **encoding)
    assert res.stdout == 'test\n'
    assert res.stderr is None

    res = client.run(['echo', 'test'], stdout=ssh.PIPE, stderr=ssh.PIPE, **encoding)
    assert res.stdout == 'test\n'
    assert res.stderr == ''


def test_call(client):
    assert client.call(['true']) == 0
    assert client.call(['false']) == 1


def test_check_call(client):
    assert client.check_call(['true']) == 0


def test_check_call_exception(client):
    with pytest.raises(ssh.CalledProcessError) as exc_info:
        client.check_call(['false'])
    assert exc_info.type is ssh.CalledProcessError
    err = exc_info.value
    assert err.returncode == 1
    assert err.output is None
    assert err.stdout is None
    assert err.stderr is None


def test_check_output(client):
    assert client.check_output(['true']) == b''
    assert client.check_output(['true'], **encoding) == ''

    assert client.check_output(['echo', 'test']) == b'test\n'
    assert client.check_output(['echo', 'test'], **encoding) == 'test\n'

    assert client.check_output(['echo', '1', '2', '3']) == b'1 2 3\n'
    assert client.check_output(['echo', '1', '2', '3'], **encoding) == '1 2 3\n'

    assert client.check_output(['echo', '1  2  3']) == b'1  2  3\n'
    assert client.check_output(['echo', '1  2  3'], **encoding) == '1  2  3\n'

    assert client.check_output(['seq', '10'], **encoding) == ''.join([str(x)+'\n' for x in range(1, 11)])
    assert client.check_output(['seq', '10000'], **encoding) == ''.join([str(x)+'\n' for x in range(1, 10001)])


def test_check_output_exception(client):
    with pytest.raises(ssh.CalledProcessError) as exc_info:
        client.check_call(['false'], stdout=ssh.PIPE, stderr=ssh.PIPE)
    assert exc_info.type is ssh.CalledProcessError
    err = exc_info.value
    assert err.returncode == 1
    assert err.output == b''
    assert err.stdout == b''
    assert err.stderr == b''


def test_input(client):
    assert client.check_output(['cat'], input=b'test\n') == b'test\n'
    assert client.check_output(['cat'], input='test\n', **encoding) == 'test\n'


def test_output_stderr(client):
    assert client.check_output(['python', '-c', 'import sys; sys.stderr.write("test")'], stderr=ssh.STDOUT) == b'test'


def test_devnull(client):
    assert client.check_call(['true'], stdin=ssh.DEVNULL, stdout=ssh.DEVNULL, stderr=ssh.DEVNULL) == 0
    assert client.check_output(['cat'], stdin=ssh.DEVNULL) == b''
    assert client.check_output(['echo', 'test'], stdin=ssh.DEVNULL) == b'test\n'
    assert client.check_call(['seq', '10000'], stdin=ssh.DEVNULL, stdout=ssh.DEVNULL, stderr=ssh.DEVNULL) == 0


def test_env(client):
    assert client.check_output(['echo', '$A'], env={'A': '123'}, **encoding) == '$A\n'
    assert client.check_output('echo $A', env={'A': '123'}, shell=True, **encoding) == '123\n'
    assert 'A=123' in client.check_output(['env'], env={'A': '123'}, **encoding).splitlines()
    assert 'A=123' in client.check_output(['env'], env={'A': '123'}, shell=True, **encoding).splitlines()


def test_shell(client):
    client.check_call(['true', 'foo'])
    with pytest.raises(Exception):
        client.check_call('true foo')
    client.check_call('true foo', shell=True)
    client.check_call(['true', 'foo'], shell=True)


def test_cwd(client):
    assert client.check_output(['pwd'], **encoding) != '/\n'
    assert client.check_output(['pwd'], cwd='/', **encoding) == '/\n'


def test_executable(client):
    assert client.check_output(['__xxx__', '/proc/self/cmdline'], executable='/bin/cat', **encoding) == '__xxx__\0/proc/self/cmdline\0'


def test_file_not_found(client, hostname):
    with pytest.raises(Exception):
        client.call(['__xxx__'])
    with pytest.raises(Exception):
        assert client.call(['cat', '/proc/self/cmdline'], executable='/bin/__xxx__')
    if hostname:
        client.call(['__xxx__'], check=False)
    with pytest.raises(Exception):
        client.call(['__xxx__'], check=True)


def test_timeout(client, hostname):
    with pytest.raises(ssh.TimeoutExpired) as exc_info:
        client.call(['sleep', '5'], timeout=1)
    assert exc_info.type is ssh.TimeoutExpired
    err = exc_info.value
    assert err.cmd == ['sleep', '5']
    assert err.output is None
    assert err.stdout is None
    assert err.stderr is None
    # seems like bug in python3 subprocess
    assert err.timeout == 1 or not hostname


def test_connect_timeout():
    client = ssh.SshClient(hostname='localhost', connect_timeout=0.5, keepalive=2)
    assert client.call(['sleep', '1']) == -1

    client = ssh.SshClient(hostname='localhost', connect_timeout=0.5, keepalive=0.2)
    assert client.call(['sleep', '1']) == 0

    with pytest.raises(Exception):
        ssh.SshClient(hostname='localhost', port=21, connect_timeout=0.5).connect()


def test_private_key():
    with pytest.raises(Exception):
        ssh.SshClient(hostname='localhost', look_for_keys=False).connect()

    with open(os.path.expanduser('~/.ssh/id_rsa'), 'rb') as f:
        private_key = f.read()
    ssh.SshClient(hostname='localhost', private_key=private_key).connect()

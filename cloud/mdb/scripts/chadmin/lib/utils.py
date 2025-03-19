import json
import subprocess


def execute(command, *, host=None, input=None, use_pssh=True, binary=False):
    if host:
        ssh = 'pssh' if use_pssh else 'ssh'
        command = f'{ssh} {host} -- {command}'

    proc = subprocess.Popen(command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    if isinstance(input, str):
        input = input.encode()

    stdout, stderr = proc.communicate(input=input)

    if proc.returncode:
        raise RuntimeError('"{0}" failed with: {1}'.format(command, stderr.decode()))

    if binary:
        return stdout
    else:
        return stdout.decode().strip()


def upload_to_paste(data):
    return execute(command='ya paste', input=data).strip()


def upload_to_mds(data):
    output = execute(command='ya upload --mds --ttl 730 --stdin-tag output.txt --json-output', input=data)
    return json.loads(output)['download_link']


def upload_to_sandbox(data):
    output = execute(command='ya upload --ttl 730 --stdin-tag output.txt --json-output', input=data)
    return json.loads(output)['download_link']

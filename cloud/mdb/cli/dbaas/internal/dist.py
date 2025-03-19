import json
import subprocess


def dist_find(name, repository=None, env=None, version=None, raw_output=False):
    options = [f'-n {name}']
    if repository:
        options.append(f'-r {repository}')
    if env:
        options.append(f'-e {env}')
    if version:
        options.append(f'-v {version}')
    if not raw_output:
        options.append('-j')

    result = _execute(f'find_package {" ".join(options)}')
    if raw_output:
        return result

    if result.startswith('No packages found'):
        return []

    return json.loads(result.replace('\n', '')).get('result', [])


def dist_move(name, version, repository, source_env, target_env):
    return _execute(f'sudo dmove "{repository}" "{target_env}" "{name}" "{version}" "{source_env}"')


def dist_copy(name, version, source_repository, target_repository, target_env):
    return _execute(f'sudo bmove -c "{target_env}" "{target_repository}" "{name}" "{version}" "{source_repository}"')


def _execute(cmd):
    cmd = f'ssh dupload.dist.yandex.net {cmd}'

    proc = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    stdout, stderr = proc.communicate()

    if proc.returncode:
        msg = f'`{cmd}` failed'
        if stderr:
            msg += ' with: ' + stderr.decode()
        elif stdout:
            msg += ' with: ' + stdout.decode()
        raise RuntimeError(msg)

    return stdout.decode()

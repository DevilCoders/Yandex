#!/usr/bin/env python

import logging
import os
from salt.exceptions import CommandExecutionError

log = logging.getLogger(__name__)

PORTO_AGENT_BIN = "/usr/sbin/mdb-porto-agent"


def __virtual__():
    return os.path.isfile(PORTO_AGENT_BIN) and os.access(PORTO_AGENT_BIN, os.X_OK)


def run(command, container, flags, stdin):
    cmd = [PORTO_AGENT_BIN]
    cmd += [command]
    cmd += ['--logshowall']
    cmd += ['-t', container]
    cmd += flags

    result = __salt__['cmd.run_all'](cmd, cwd='/', stdin=stdin)
    result['stdout'] = result['stdout'].encode('ascii', 'replace')
    result['stderr'] = result['stderr'].encode('ascii', 'replace')

    if result['retcode'] != 0:
        raise CommandExecutionError(' '.join(cmd) + '\nstdout:\n' + result['stdout'] + '\nstderr:\n' + result['stderr'])
    return {'result': True, 'out': result['stdout'], 'logs': result['stderr'].split('\n')}

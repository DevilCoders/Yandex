"""
filex - extensions to file state.

We could keep the original name but better keep things
explicit. As an added bonus, we can mix file and filex
under one id now.
"""

import hashlib
import logging
import os
import subprocess
import time

log = logging.getLogger(__name__)


def from_command(name, cmd=None, cwd=None, makedirs=False, quiet=False, retries=1, retry_backoff=15):
    """
    Like file.managed, but captures the commands output.

    If the command returns non-zero exit code, state fails.
    If file is changed on disk, the state is considered changed.

    Intended usage:

    some_id:
      file.from_command:
        - name: /tmp/net_config
        - cmd: zcat /proc/config.gz | grep ^NET_
    """
    ret = {
        'name': name,
        'changes': {},
        'result': None,
        'comment': ''
    }

    dirname = os.path.dirname(name)
    if not os.path.exists(dirname):
        if makedirs:
            os.makedirs(dirname)
        else:
            ret['comment'] = '{} doesn\'t exist'.format(dirname)
            ret['result'] = False
            return ret

    if os.path.exists(name):
        with open(name, 'r') as f:
            current = f.read()
    else:
        current = None

    last_error = None
    for i in xrange(retries):
        try:
            output = subprocess.check_output(cmd, shell=True, cwd=cwd)
            break
        except subprocess.CalledProcessError as e:
            last_error = str(e)
            log.error("%s, retrying in %d seconds...", e, retry_backoff)

        time.sleep(retry_backoff)
    else:
        ret['comment'] = last_error
        if not __opts__['test']:
            ret['result'] = False
        return ret


    if output != current:
        if not __opts__['test']:
            with open(name, 'w') as f:
                f.write(output)

            if not quiet:
                ret['changes'][name] = {
                    'old': current,
                    'new': output
                }
            else:
                ret['changes'][name] = {
                    'old': hashlib.md5(current).hexdigest() if current else '<empty>',
                    'new': hashlib.md5(output).hexdigest()
                }

            ret['result'] = True
        else:
            ret['comment'] = 'Will update {}'.format(name)
    else:
        # No changes
        ret['result'] = True

    return ret

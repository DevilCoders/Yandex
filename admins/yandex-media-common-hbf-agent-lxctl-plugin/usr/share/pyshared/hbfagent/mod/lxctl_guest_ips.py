import re
from subprocess import check_output, CalledProcessError

def lxctl_list():
    try:
        out = check_output(['sudo', 'lxc-ls', '-f'], universal_newlines=True)
        return out.split(u'\n')
    except CalledProcessError:
        return  None


def run():
    containers = lxctl_list()
    ip6 = set()
    ip4 = set()

    if containers is None:
        return []

    for line in containers:
        for word in line.split():
            match = re.match(r'([0-9a-f]+:[0-9a-f:]+)', word)
            if match:
                ip6.add(match.group(1))
                continue

            match = re.match(r'((?:\d+\.){3}\d+)', word)
            if match:
                ip4.add(match.group(1))
                continue

    return (ip6.union(ip4))

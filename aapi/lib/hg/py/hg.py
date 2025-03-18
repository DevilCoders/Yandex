import re
import hglib.client as hgclient


def identify(name, repository):
    client = hgclient.hgclient(None, None, None)
    ident_output = client.identify(rev=name, source=repository, id=True, debug=True)

    hashes = []
    for line in ident_output.strip().split('\n'):
        line = line.strip()

        m = re.match(r'^([0-9a-f]{40})$', line)

        if m is not None:
            hashes.append(m.group(0))

    if len(hashes) == 0:
        raise Exception('Can\'t find hash in hg id output\n{}'.format(ident_output))

    if len(hashes) > 1:
        raise Exception('Hg id output is ambiguous\n{}'.format(ident_output))

    return hashes[0]

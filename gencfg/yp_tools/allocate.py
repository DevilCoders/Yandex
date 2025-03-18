import datetime
import json
import logging
import os
import subprocess
import sys

import core.db
from make_recluster_request import partial_recluster, group, group_requirements
from yp_to_gencfg import main as generate


def main(groups):
    masters = set(group(g).card.master.card.name for g in groups)
    if len(masters) > 1:
        raise Exception('Groups are from different masters: {}'.format(masters))

    master = list(masters)[0]
    logging.info('Master: %s', master)

    workdir = os.path.join('generated', 'allocator', datetime.datetime.now().isoformat())
    logging.info('Working directory: %s', workdir)
    os.makedirs(workdir)
    # os.symlink(workdir, os.path.join('generated', 'allocator', 'latest'))

    js = os.path.join(workdir, 'allocate.json')
    with open(js, 'w') as f:
        data = partial_recluster(master=master,
                                 relocate=[group(g) for g in groups],
                                 wipe=[],
                                 backgrounds=[],
                                 )
        json.dump(sorted(data), f, indent=4, sort_keys=True)

    null = open(os.devnull, 'w')
    subprocess.call(['/home/okats/svn/arcadia/search/tools/yp_alloc/yp_alloc', js, js], stderr=null)

    result_dir = os.path.join(workdir, 'result')
    os.makedirs(result_dir)
    generate(js + '_0', result_dir)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG, format='[%(asctime)s] %(message)s')
    main(sys.argv[1].split(','))


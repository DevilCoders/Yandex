"""
Simple script for update security groups
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.tasks.common.deploy import BaseDeployExecutor
from cloud.mdb.dbaas_worker.internal.tasks.utils import build_host_group


CTYPE_HOST_GROUPS = {
    'postgresql': ['postgresql'],
    'clickhouse': ['clickhouse', 'zookeeper'],
    'mongodb': ['mongod', 'mongos', 'mongocfg', 'mongoinfra'],
    'mysql': ['mysql'],
    'redis': ['redis'],
    'elasticsearch': ['elasticsearch_data', 'elasticsearch_master'],
    'kafka': ['kafka', 'zookeeper'],
    'sqlserver': ['sqlserver'],
}


def update_sgroups():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('-n', '--dry-run', action='store_true', help='Only emulate, do not real update')
    parser.add_argument('-e', '--env', type=str, help='update only specified environment')
    parser.add_argument('-l', '--limit', type=int, default=1, help='limit update cluster count with security groups')
    parser.add_argument('--cid', type=str, help='update only specified cluster')
    parser.add_argument('ctype', type=str, help='update only cluster type', choices=CTYPE_HOST_GROUPS.keys())
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')

    dry_prefix = "because dry run do nothing, " if args.dry_run else ""
    list_task = {
        'cid': 'list-cids',
        'task_id': 'list-cids',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }
    config = get_config(args.config)
    base_exec = BaseDeployExecutor(config, list_task, Queue(), {})
    metadb = BaseMetaDBProvider(config, list_task, Queue())

    any_host_opts = {'any': {'vtype': "any"}}
    hgs = [build_host_group(config.__getattribute__(hg), any_host_opts) for hg in CTYPE_HOST_GROUPS[args.ctype]]
    # rules
    base_service_rules = base_exec.sg_service_rules(hgs, False)
    logging.info('use security group base rules: %r' % base_service_rules)
    allow_all_service_rules = base_exec.sg_service_rules(hgs, True)
    logging.info('use security group allow all rules: %r' % allow_all_service_rules)
    # hashes
    base_hash = base_exec.sg_calc_hash(base_service_rules)
    allow_all_hash = base_exec.sg_calc_hash(allow_all_service_rules)
    logging.info('got security group rules hash. Base: %d, Allow all: %s' % (base_hash, allow_all_hash))

    ctype = '%s_cluster' % args.ctype
    list_cids = get_cid_for_service_sg_update(
        metadb=metadb, ctype=ctype, env=args.env, limit=args.limit, sg_hashes=[base_hash, allow_all_hash], cid=args.cid
    )
    logging.info('* got list for security group update: %s' % list_cids)
    for cid, sg_id, sg_hash, allow_all in list_cids:
        rules = base_service_rules
        hash = base_hash
        if allow_all:
            rules = allow_all_service_rules
            hash = allow_all_hash
        if sg_hash == hash:
            continue
        logging.info("%supdate cluster '%s' service security group '%s' rules" % (dry_prefix, cid, sg_id))
        logging.info("cluster '%s' service security group hash '%s' != '%s'" % (cid, sg_hash, hash))
        if not args.dry_run:
            update_sg_with_rules(config, cid, sg_id, rules, hash, allow_all)


def update_sg_with_rules(config, cid, sg_id, rules, sg_hash, sg_allow_all):
    """
    Update service security group sg_id for cluster cid
    """
    update_task = {
        'cid': cid,
        'task_id': 'update-service-sg',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'some',
    }
    update = BaseDeployExecutor(config, update_task, Queue(), {})
    update.sg_update_rules(sg_id, rules, sg_hash, sg_allow_all)


def get_cid_for_service_sg_update(metadb, ctype, env, limit, sg_hashes, cid=None):
    """
    Get resources existing in metadb (only hosts for now)
    """
    ret = {}
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            cur.execute(
                """
                SELECT cid, sg_ext_id, sg_hash, sg_allow_all
                FROM dbaas.clusters c
                JOIN dbaas.sgroups sg USING (cid)
                WHERE code.managed(c)
                  AND code.visible(c)
                  AND c.type = %(ctype)s
                  AND (%(env)s IS NULL OR c.env = %(env)s)
                  AND sg_type = 'service'
                  AND sg_hash NOT IN (SELECT unnest(%(sg_hashes)s))
                  AND (%(cid)s IS NULL OR cid = %(cid)s)
                LIMIT %(limit)s
                """,
                {
                    'ctype': ctype,
                    'env': env,
                    'limit': limit,
                    'sg_hashes': sg_hashes,
                    'cid': cid,
                },
            )
            ret = [(row['cid'], row['sg_ext_id'], row['sg_hash'], row['sg_allow_all']) for row in cur.fetchall()]
    return ret

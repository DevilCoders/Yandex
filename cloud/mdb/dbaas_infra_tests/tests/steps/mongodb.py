"""
MongoDB related steps
"""
from copy import deepcopy
from datetime import datetime

import bson
import yaml
from behave import register_type, then, when
from hamcrest import (assert_that, empty, equal_to, has_key, has_length, is_not, starts_with)
from parse_type import TypeBuilder
from pymongo import WriteConcern
from retrying import retry

from tests.helpers import internal_api
from tests.helpers.crypto import gen_plain_random_string
from tests.helpers.metadb import get_subclusters_shards_by_cid
from tests.helpers.mongodb import (MONGO_ADMIN_USER, MONGOCFG_HOST_TYPE, MONGOD_DEFAULT_SHARD, MONGOD_HOST_TYPE,
                                   MONGOINFRA_HOST_TYPE, MONGOS_HOST_TYPE, OplogArchive, OplogTimestamp,
                                   mongo_get_conns, mongo_get_primary_conn, wait_secondaries_sync)
from tests.helpers.step_helpers import (get_step_data, get_timestamp, step_require, store_timestamp_value)
from tests.helpers.utils import merge
from tests.steps.s3 import _get_external_s3_client

register_type(
    ShardKeyType=TypeBuilder.make_enum({
        'ranged': 1,
        'hashed': 'hashed',
    }),
    HostType=TypeBuilder.make_enum({
        'MONGOD': MONGOD_HOST_TYPE,
        'MONGOCFG': MONGOCFG_HOST_TYPE,
        'MONGOS': MONGOS_HOST_TYPE,
        'MONGOINFRA': MONGOINFRA_HOST_TYPE,
    }),
)


@then('ns "{namespace}" has "{chunk_count:d}" chunks on shard "{shard_name}"')
@retry(wait_fixed=1000, stop_max_attempt_number=120)
def step_count_ns_chunks_on_shard(context, namespace, chunk_count, shard_name):
    connections = mongo_get_conns(
        context,
        MONGO_ADMIN_USER,
        [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE],
    )
    mclient = mongo_get_primary_conn(connections)['mc']
    current_count = mclient['config']['chunks'].find({'ns': namespace, 'shard': shard_name}).count()
    assert_that(current_count, equal_to(chunk_count))


@then('mongodb {host_type:HostType} "{shard_name:w}" shard replset in cluster "{cluster_name:w}" '
      'has {rcount:d} members')
@then('mongodb {host_type:HostType} replset in cluster "{cluster_name:w}" has {rcount:d} members')
@retry(wait_fixed=1000, stop_max_attempt_number=120)
def step_mongodb_rs_members_count(context, cluster_name, rcount, host_type, shard_name=None):
    """
    Connect to each host and count statuses
    """
    internal_api.load_cluster_into_context(context, cluster_name)
    connections = mongo_get_conns(context, MONGO_ADMIN_USER, host_type, shard_name)
    for conn in connections:
        rs_conf = conn['mc'].local.system.replset.find_one()
        assert_that(rs_conf, has_key('_id'), 'Bad replset config')
        assert_that(rs_conf['members'], has_length(rcount))


@then('mongodb "{cluster_name:w}" options has entry')
@step_require('cluster_config', 'folder')
@retry(wait_fixed=1000, stop_max_attempt_number=10)
def step_mongodb_get_config(context, cluster_name):
    internal_api.load_cluster_into_context(context, cluster_name)
    settings = yaml.load(context.text)

    for host_type in settings:
        connections = mongo_get_conns(context, MONGO_ADMIN_USER, host_type)
        for conn in connections:
            config = conn['mc'].get_database('admin').command({
                'getCmdLineOpts': 1,
            })

            expected = deepcopy(config['parsed'])
            merge(expected, settings.get(host_type, {}))

            assert_that(config['parsed'], equal_to(expected),
                        'Unexpected output on host {0}'.format(conn['fqdn_port']))


@then('mongodb major version is "{version}"')
@step_require('cluster_config', 'folder')
@retry(wait_fixed=1000, stop_max_attempt_number=10)
def step_mongodb_check_version(context, version):
    connections = mongo_get_conns(context, MONGO_ADMIN_USER)
    for conn in connections:
        build_info = conn['mc'].get_database('admin').command({
            'buildInfo': 1,
        })

        assert_that(build_info['version'], starts_with(version), 'Unexpected output on host {0}: {1}'.format(
            conn['fqdn_port'], build_info['version']))


@when('{user} inserts {count:d} test mongodb docs into "{database:w}.{collection:w}"')
@when('{user} inserts {count:d} test mongodb docs into "{database:w}.{collection:w}" {host_type:HostType}')
@when('{user} inserts {count:d} test mongodb docs into "{database:w}.{collection:w}" {host_type:HostType} '
      '"{shard_name:w}"')
@step_require('cluster_config')
def step_mongo_insert_test_data(context, user, count, database, collection, host_type=None, shard_name=None):
    # pylint: disable=too-many-arguments
    if host_type is None:
        host_type = MONGOD_HOST_TYPE
    if shard_name is None and host_type == MONGOD_HOST_TYPE:
        shard_name = MONGOD_DEFAULT_SHARD

    connections = mongo_get_conns(context, user, host_type, shard_name)
    mclient = mongo_get_primary_conn(connections)['mc']
    for i in range(count):
        mclient[database][collection].with_options(write_concern=WriteConcern(w="majority")).insert_one({
            'num': i,
            'dt': datetime.utcnow(),
            'text': gen_plain_random_string(16),
        })

    if host_type not in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
        wait_secondaries_sync(mongo_get_conns(context, MONGO_ADMIN_USER, host_type, shard_name))


@then('mongodb databases list is equal to following list')
@then('mongodb databases {host_type:HostType} on "{shard_name:w}" and list is equal to following list')
@then('mongodb databases on "{shard_name:w}" and list is equal to following list')
@step_require('cluster_config')
def step_mongo_list_databases(context, host_type=None, shard_name=None):
    # pylint: disable=too-many-arguments
    if host_type is None:
        host_type = MONGOD_HOST_TYPE
    if shard_name is None and host_type == MONGOD_HOST_TYPE:
        shard_name = MONGOD_DEFAULT_SHARD

    connections = mongo_get_conns(context, 'admin', host_type, shard_name)
    mclient = mongo_get_primary_conn(connections)['mc']
    db_list = mclient.list_database_names()
    target_list = get_step_data(context)
    db_list.sort()
    target_list.sort()
    assert_that(db_list, equal_to(target_list))


@when('{user} enables sharding of database "{database}"')
def step_enable_database_sharding(context, user, database):
    connections = mongo_get_conns(context, user, [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE])
    mclient = mongo_get_primary_conn(connections)['mc']
    cmd_kwargs = dict(command=bson.son.SON([('enableSharding', database)]), check=True)
    cmd = mclient['admin'].command(**cmd_kwargs)
    assert_that(cmd['ok'], equal_to(1))


@when('{user} shards "{namespace}" with key "{shard_key}" and type "{shard_type:ShardKeyType}"')
def step_shard_namespace(context, user, namespace, shard_key, shard_type):
    connections = mongo_get_conns(context, user, [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE])
    mclient = mongo_get_primary_conn(connections)['mc']
    cmd_kwargs = dict(
        command=bson.son.SON([('shardCollection', namespace), ('key', {
            shard_key: shard_type,
        }), ('numInitialChunks', 4)]),
        check=True)
    cmd = mclient['admin'].command(**cmd_kwargs)
    assert_that(cmd['ok'], equal_to(1))


@then('{user} has {count:d} test mongodb docs in "{database:w}.{collection:w}"')
@then('{user} has {count:d} test mongodb docs in "{database:w}.{collection:w}" {host_type:HostType}')
@then('{user} has {count:d} test mongodb docs in "{database:w}.{collection:w}" {host_type:HostType} "{shard_name:w}"')
@step_require('cluster_config')
def step_mongod_count_test_data(context, user, count, database, collection, host_type=None, shard_name=None):
    # pylint: disable=too-many-arguments
    if host_type is None:
        host_type = MONGOD_HOST_TYPE
    if shard_name is None and host_type == MONGOD_HOST_TYPE:
        shard_name = MONGOD_DEFAULT_SHARD

    connections = mongo_get_conns(context, user, host_type, shard_name)
    assert_that(connections, is_not(empty()))
    for conn in connections:
        mclient = conn['mc']
        assert_that(mclient[database][collection].count(), equal_to(count), "Host: {0}, role: {1}, type: {2}".format(
            conn['fqdn_port'], conn['role'], conn['host_type']))


@then('mongodb {host_type:HostType} "{shard_name:w}" shard replset in cluster "{cluster_name:w}" is synchronized')
@then('mongodb {host_type:HostType} replset in cluster "{cluster_name:w}" is synchronized')
@when('mongodb {host_type:HostType} "{shard_name:w}" shard replset in cluster "{cluster_name:w}" is synchronized')
@when('mongodb {host_type:HostType} replset in cluster "{cluster_name:w}" is synchronized')
def step_mongodb_rs_synchronized(context, cluster_name, host_type=None, shard_name=None):
    """
    Connect to each host and check opdate
    """
    internal_api.load_cluster_into_context(context, cluster_name)
    connections = mongo_get_conns(context, MONGO_ADMIN_USER, host_type, shard_name)
    wait_secondaries_sync(connections)


@then('{user} moves mongodb primary databases from "{src_shard:w}" to "{dst_shard:w}"')
@step_require('cluster_config')
def step_mongos_move_primary_dbs_from_shard(context, user, src_shard, dst_shard):
    connections = mongo_get_conns(context, user, [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE])
    mclient = connections.pop()['mc']
    config_db = mclient.get_database('config')
    if 'databases' not in config_db.collection_names():
        return

    # move db primary
    admin_db = mclient.get_database('admin')
    for db in config_db['databases'].find({'primary': src_shard}):
        admin_db.command({'movePrimary': db['_id'], 'to': dst_shard}, check=True)


@then('{user} flushes MONGOS cache')
@step_require('cluster_config')
def step_mongos_flush_cache(context, user):
    connections = mongo_get_conns(context, user, [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE])
    for conn in connections:
        admin_db = conn['mc'].get_database('admin')
        admin_db.command('flushRouterConfig')


@then('mongodb featureCompatibilityVersion is "{fcv}"')
@step_require('cluster_config')
def step_mongo_check_fcv(context, fcv):
    connections = mongo_get_conns(context, MONGO_ADMIN_USER)
    for conn in connections:
        if conn['host_type'] in [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE]:
            continue
        admin_db = conn['mc'].get_database('admin')
        cmd_ret = admin_db.command({'getParameter': 1, 'featureCompatibilityVersion': 1})
        return assert_that(cmd_ret['featureCompatibilityVersion']['version'], equal_to(fcv))


@when('we save shard "{shard}" last oplog timestamp to "{ts_key}"')
@step_require('cluster_config')
def step_mongo_get_last_maj_ts(context, shard, ts_key):
    connections = mongo_get_conns(context, MONGO_ADMIN_USER, MONGOD_HOST_TYPE, shard)
    local_db = mongo_get_primary_conn(connections)['mc'].get_database('local')
    cmd = local_db.command({'isMaster': 1})
    assert_that(cmd['ok'], equal_to(1))
    # restore point is rounded here
    # we take next second after current due to resolve issues with multiple 'inc's in one second
    store_timestamp_value(context, ts_key, cmd['lastWrite']['majorityOpTime']['ts'])


@when('we wait until shard "{shard_name}" timestamp next to "{ts_key}" appears in storage')
@retry(wait_fixed=1000, stop_max_attempt_number=30)
def step_wait_oplog_ts_archiving(context, shard_name, ts_key):
    bson_ts = get_timestamp(context, ts_key)
    target_ts = OplogTimestamp(ts=bson_ts.time, inc=bson_ts.inc)
    s3_client, s3_bucket = _get_external_s3_client(context)
    subcids_shards = get_subclusters_shards_by_cid(context, context.cluster['id'])
    shard_id = next(iter(shard['shard_id'] for shard in subcids_shards if shard['shard_name'] == shard_name))
    path = "mongodb-backup/{cid}/{shard_id}/oplog_005/".format(cid=context.cluster['id'], shard_id=shard_id)
    objects = s3_client.list_objects(Bucket=s3_bucket, Delimiter='/', Prefix=path).get('Contents', [])

    for obj in objects:
        if OplogArchive.from_string(obj['Key']).includes_ts(target_ts):
            return
    raise RuntimeError('Target timestamp "{0}"({1}) was not found at path {2}'.format(ts_key, target_ts, path))

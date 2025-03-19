"""
Execute steps
"""
import difflib
import logging
import uuid
from collections.abc import Mapping
from contextlib import contextmanager
from random import randint

import jinja2
import jinja2.meta
import yaml
from behave import given, then, when
from hamcrest import (
    all_of,
    assert_that,
    contains_inanyorder,
    contains_string,
    empty,
    equal_to,
    has_entries,
    has_length,
    has_properties,
    has_property,
    is_,
    matches_regexp,
)
from hamcrest.core.core.isequal import IsEqual
from psycopg2 import DatabaseError
from psycopg2 import Error as PGError

from cloud.mdb.dbaas_metadb.tests.helpers.cluster_changes import (
    cluster_change,
    complete_change,
    in_transaction,
    lock_cluster,
    complete_future_change,
    lock_future_cluster,
)
from cloud.mdb.dbaas_metadb.tests.helpers.connect import connect
from cloud.mdb.dbaas_metadb.tests.helpers.queries import (
    execute_query,
    execute_query_in_transaction,
    translate_query_vars,
    verbose_query,
)
from cloud.mdb.dbaas_metadb.tests.helpers.types import Enum

# pylint: disable=missing-docstring


def make_uuid() -> str:
    return str(uuid.uuid1())


def make_new_name(prefix: str) -> str:
    return prefix + '-' + make_uuid()


def rnd_id():
    return randint(0, 2**32)


ADD_CLOUD_Q = verbose_query(
    """
SELECT cloud_id
  FROM code.add_cloud(
      i_cloud_ext_id => :cloud_ext_id,
      i_quota        => code.make_quota(
          i_cpu       => :cpu_quota,
          i_memory    => :memory_quota,
          i_ssd_space => :ssd_space_quota,
          i_hdd_space => :hdd_space_quota,
          i_clusters  => :clusters_quota
      ),
      i_x_request_id => 'tests.request'
  )
"""
)

ADD_FOLDER_Q = verbose_query(
    """
INSERT INTO dbaas.folders
    (cloud_id, folder_ext_id)
VALUES
    (:cloud_id, :folder_ext_id)
RETURNING *
"""
)

GET_REV_Q = verbose_query(
    """
SELECT actual_rev FROM dbaas.clusters WHERE cid = :cid
"""
)


def create_cloud(context, quota):
    """
    Create new cloud
    """
    cloud_ext_id = 'cloud{}'.format(make_uuid())
    cloud_args = {'cloud_ext_id': cloud_ext_id}
    cloud_args.update(quota)

    with execute_query(context.dsn, ADD_CLOUD_Q, cloud_args) as res:
        cloud_id = res.fetch()[0].cloud_id
    return cloud_id, cloud_ext_id


_DEFAULT_QUOTA = {
    'cpu_quota': 48.0,
    'memory_quota': 206158430208,
    'clusters_quota': 40,
    'ssd_space_quota': 3298534883328,
    'hdd_space_quota': 3298534883328,
}


@given('cluster type pillar')
def step_cluster_type_pillar(context):
    args = yaml.safe_load(context.text)
    with execute_query(
        context.dsn,
        '''
            INSERT INTO dbaas.cluster_type_pillar (type, value)
            VALUES (%(type)s, %(value)s)
            ON CONFLICT (type) DO UPDATE
            SET value = %(value)s
            ''',
        args,
    ):
        pass


@given('role pillar')
def step_role_pillar(context):
    args = yaml.safe_load(context.text)
    with execute_query(
        context.dsn,
        '''
            INSERT INTO dbaas.role_pillar (type, role, value)
            VALUES (%(type)s, %(role)s, %(value)s)
            ON CONFLICT (type, role) DO UPDATE
            SET value = %(value)s
            ''',
        args,
    ):
        pass


@given('cloud with quota')
@given('cloud with default quota')
@when('I create new cloud with quota')
def step_make_cloud(context):
    quota = _DEFAULT_QUOTA
    if context.text:
        quota = yaml.safe_load(context.text)
    context.cloud_id, context.cloud_ext_id = create_cloud(
        context,
        quota,
    )


def make_folder(context):
    if 'cloud_id' not in context:
        raise RuntimeError('No cloud_id in context. Create cloud first.')
    folder_args = {
        'cloud_id': context.cloud_id,
        'folder_ext_id': 'folder{}'.format(make_uuid()),
    }
    with execute_query(context.dsn, ADD_FOLDER_Q, folder_args) as res:
        return res.fetch()[0].folder_id


@given('folder')
def step_make_folder(context):
    context.folder_id = make_folder(context)


@given('other folder')
def step_make_other_folder(context):
    context.other_folder_id = make_folder(context)


def make_cluster_args(context):
    return {
        'name': make_new_name('first-metadb-test-cluster-'),
        'type': 'postgresql_cluster',
        'cid': context.cid,
        'folder_id': context.folder_id,
        'network_id': '',
        'description': None,
        'public_key': '',
        'env': 'qa',
        'x_request_id': 'cluster.create.request',
    }


CREATE_CLUSTER_Q = verbose_query(
    """
    SELECT * FROM code.create_cluster(
        i_cid          => :cid,
        i_name         => :name,
        i_type         => :type,
        i_env          => :env,
        i_public_key   => :public_key,
        i_network_id   => :network_id,
        i_folder_id    => :folder_id,
        i_description  => :description,
        i_x_request_id => :x_request_id
    )
"""
)

ADD_PILLAR_Q = verbose_query(
    """
    SELECT * FROM code.add_pillar(
        i_cid    => :cid,
        i_rev    => :rev,
        i_value  => :value,
        i_key    => code.make_pillar_key(
            i_cid      => :pillar_cid,
            i_subcid   => :pillar_subcid,
            i_shard_id => :pillar_shard_id,
            i_fqdn     => :pillar_fqdn
        )
    )
"""
)

ADD_SUBCLUSTER_Q = verbose_query(
    """
    SELECT * FROM code.add_subcluster(
        i_cid    => :cid,
        i_subcid => :subcid,
        i_name   => :name,
        i_roles  => :roles,
        i_rev    => :rev
    )
"""
)

ADD_SHARD_Q = verbose_query(
    """
    SELECT * FROM code.add_shard(
        i_subcid    => :subcid,
        i_shard_id  => :shard_id,
        i_name      => :name,
        i_cid       => :cid,
        i_rev       => :rev
    )
"""
)

ADD_HOST_Q = verbose_query(
    """
    SELECT * FROM code.add_host(
        i_subcid           => :subcid,
        i_shard_id         => :shard_id,
        i_space_limit      => 42::bigint,
        i_flavor_id        => :flavor_id,
        i_geo              => 'dc1',
        i_fqdn             => :fqdn,
        i_disk_type        => 'disk_type1',
        i_subnet_id        => 'dummy_subnet',
        i_assign_public_ip => false,
        i_cid              => :cid,
        i_rev              => :rev
    )
"""
)

ADD_TARGET_PILLAR_Q = verbose_query(
    """
INSERT INTO dbaas.target_pillar
    (target_id, cid, subcid, shard_id, fqdn, value)
VALUES
    (:target_id, :cid, :subcid, :shard_id, :fqdn, :value)
"""
)


@given('cluster')
@given('cluster with')
@given('"{cluster_prefix:w}" cluster with')
def step_add_cluster(context, cluster_prefix=None):
    step_cluster_args = yaml.safe_load(context.text) if context.text else {}
    context.cid = step_cluster_args.get('cid', make_uuid())
    cluster_args = make_cluster_args(context)
    cluster_args.update(step_cluster_args)

    with in_transaction(context) as trans:
        execute_query_in_transaction(trans, CREATE_CLUSTER_Q, cluster_args)
        complete_change(trans, context.cid, 1)

    if cluster_prefix:
        for attr_name in ['cid', 'created_at', 'name']:
            if attr_name in cluster_args:
                setattr(context, cluster_prefix + '_' + attr_name, cluster_args[attr_name])


@given('cluster with pillar')
def step_cluster_with_pillar(context):
    context.cid = make_uuid()

    with in_transaction(context) as trans:
        execute_query_in_transaction(trans, CREATE_CLUSTER_Q, make_cluster_args(context))
        execute_query_in_transaction(
            trans,
            ADD_PILLAR_Q,
            {
                'cid': context.cid,
                'pillar_cid': context.cid,
                'pillar_subcid': None,
                'pillar_shard_id': None,
                'pillar_fqdn': None,
                'value': context.text,
                'rev': 1,
            },
        )
        complete_change(trans, context.cid, 1)


def _make_subcluster_args(context, role, rev):
    return {
        'subcid': context.subcid,
        'cid': context.cid,
        'name': make_new_name('first-metadb-test-sibcluster-'),
        'roles': [Enum('dbaas.role_type', role)],
        'rev': rev,
    }


@given('"{role}" subcluster')
@given('subcluster')
def step_subcluster(context, role='postgresql_cluster'):
    context.subcid = make_uuid()
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(trans, ADD_SUBCLUSTER_Q, _make_subcluster_args(context, role, rev))


@given('subcluster with pillar')
def step_subcluster_with_pillar(context):
    context.subcid = make_uuid()
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(trans, ADD_SUBCLUSTER_Q, _make_subcluster_args(context, 'postgresql_cluster', rev))
        execute_query_in_transaction(
            trans,
            ADD_PILLAR_Q,
            {
                'cid': context.cid,
                'rev': rev,
                'pillar_cid': None,
                'pillar_subcid': context.subcid,
                'pillar_shard_id': None,
                'pillar_fqdn': None,
                'value': context.text or '{}',
            },
        )


@given('shard')
def step_shard(context):
    context.shard_id = make_uuid()
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(
            trans,
            ADD_SHARD_Q,
            {
                'subcid': context.subcid,
                'shard_id': context.shard_id,
                'name': make_new_name('first-metadb-test-shard-'),
                'cid': context.cid,
                'rev': rev,
            },
        )


@given('shard with pillar')
def step_shard_with_pillar(context):
    context.shard_id = make_uuid()
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(
            trans,
            ADD_SHARD_Q,
            {
                'subcid': context.subcid,
                'shard_id': context.shard_id,
                'name': make_new_name('first-metadb-test-shard-'),
                'cid': context.cid,
                'rev': rev,
            },
        )
        execute_query_in_transaction(
            trans,
            ADD_PILLAR_Q,
            {
                'pillar_shard_id': context.shard_id,
                'pillar_cid': None,
                'pillar_subcid': None,
                'pillar_fqdn': None,
                'value': context.text,
                'cid': context.cid,
                'rev': rev,
            },
        )


@contextmanager
def new_host_created(context, fqdn):
    """
    Create new host with given fqdn

    yields transaction and revision
    """
    with cluster_change(context) as (trans, rev):
        execute_query_in_transaction(
            trans,
            ADD_HOST_Q,
            {
                'subcid': context.subcid,
                'flavor_id': context.flavor_id,
                'fqdn': fqdn,
                'cid': context.cid,
                'shard_id': getattr(context, 'shard_id', None),
                'rev': rev,
            },
        )
        yield trans, rev


@given('host')
@given('"{fqdn_var:w}" host')
@when('I add host')
def step_add_host(context, fqdn_var=None):
    fqdn = getattr(context, fqdn_var or 'fqdn')
    with new_host_created(context, fqdn):
        pass


@given('"{new_count:d}" new hosts')
def step_add_new_random_host(context, new_count):
    for _ in range(new_count):
        context.execute_steps(
            """
            Given new fqdn
            And host
        """
        )


@given('"{fqdn}" host with pillar')
@given('host with pillar')
def step_host_with_pillar(context, fqdn=None):
    if fqdn is None:
        fqdn = 'fqdn-' + str(rnd_id())
    context.fqdn = fqdn

    with new_host_created(context, fqdn) as (trans, rev):
        execute_query_in_transaction(
            trans,
            ADD_PILLAR_Q,
            {
                'pillar_shard_id': None,
                'pillar_cid': None,
                'pillar_subcid': None,
                'pillar_fqdn': fqdn,
                'value': context.text,
                'cid': context.cid,
                'rev': rev,
            },
        )


@given('target_pillar id')
def step_gen_new_target_pillar_id(context):
    context.target_pillar_id = make_uuid()


FLAVOR_QUERY = verbose_query(
    """
SELECT *
  FROM dbaas.flavors
 WHERE name = :flavor_name
"""
)


def get_flavor_id(dsn, flavor_name: str) -> str:
    with execute_query(dsn, FLAVOR_QUERY, dict(flavor_name=flavor_name)) as cur:
        return cur.fetch()[0][0]


@given('"{flavor_name:Word}" flavor')
def step_get_flavor_id(context, flavor_name):
    context.flavor_id = get_flavor_id(context.dsn, flavor_name)


@given('new fqdn')
@given('new fqdn as "{fqdn_var:w}"')
def step_gen_fqdn(context, fqdn_var='fqdn'):
    setattr(context, fqdn_var, make_new_name('fqdn'))


@given('task_id')
@when('I generate new task_id')
def step_new_task_id(context):
    context.task_id = make_uuid()


@given('required_task_id')
def step_new_required_task_id(context):
    context.required_task_id = make_uuid()


@given('idempotence_id')
def step_new_idempotence_id(context):
    context.idempotence_id = make_uuid()


@given('vars "{var_names_str}"')
def step_make_new_vars(context, var_names_str):
    var_names = [v.strip() for v in var_names_str.split(',')]
    for v_name in var_names:
        setattr(context, v_name, make_uuid())


@given('actual "{cid_var:w}" rev is "{into_rev_var:w}"')
def step_get_rev(context, cid_var, into_rev_var):
    with execute_query(context.dsn, GET_REV_Q, {'cid': getattr(context, cid_var)}) as cur:
        rev = cur.fetch()[0].actual_rev
        setattr(context, into_rev_var, rev)


@given('{pillar_key:PillarKey} target_pillar')
def step_store_target_pillar(context, pillar_key):
    target_pillar_args = dict(
        target_id=context.target_pillar_id,
        value=context.text,
        **dict.fromkeys(['cid', 'subcid', 'shard_id', 'fqdn']),
    )
    target_pillar_args[pillar_key] = getattr(context, pillar_key)
    with execute_query(context.dsn, ADD_TARGET_PILLAR_Q, target_pillar_args):
        pass


def make_query_from_context(context):
    """
    Extract query from context.text and collect its vars
    """
    query, query_vars_names = translate_query_vars(context.text)
    query_vars = dict((name, getattr(context, name)) for name in query_vars_names)
    return query, query_vars


@when('I execute query')
def step_execute_query(context):
    """
    Execute query and store its results in context
    """
    # remove rows and execute_error
    # from previous step execution
    context.execute_error = None
    context.rows = None
    query, query_vars = make_query_from_context(context)
    try:
        with execute_query(context.dsn, query, query_vars) as cur:
            context.rows = cur.fetch()
    except DatabaseError as exc:
        logging.warning('Query error %s', exc)
        context.execute_error = exc


@given('new transaction')
@when('I start transaction')
def step_start_transaction(context):
    if 'trans' in context:
        raise NotImplementedError('Concurrent transactions control not implemented')
    context.trans = connect(context.dsn, application_name='I start transaction')


@when('I execute in transaction')
def step_execute_in_trans(context):
    """
    Execute query in trans and store its results in context
    """
    context.execute_error = None
    context.rows = None
    query, query_vars = make_query_from_context(context)
    try:
        context.rows = execute_query_in_transaction(context.trans, query, query_vars).fetch()
    except DatabaseError as exc:
        logging.warning('Query error %s', exc)
        context.execute_error = exc


def _free_trans(context):
    try:
        context.trans.close()
    except PGError as exc:
        logging.warning("Close error: %s", exc)
    delattr(context, 'trans')


@when('I commit transaction')
def step_do_commit(context):
    context.execute_error = None
    context.rows = None
    try:
        context.rows = context.trans.commit()
    except DatabaseError as exc:
        logging.warning('Query error %s', exc)
        context.execute_error = exc
    _free_trans(context)


@then('I rollback transaction')
def step_do_rollback(context):
    try:
        context.trans.rollback()
    except PGError as exc:
        logging.warning("Rollback error %s", exc)
    _free_trans(context)


@when('I lock cluster')
def step_lock_cluster(context):
    context.rev = lock_cluster(context.trans, context.cid)


@when('I complete cluster change')
def step_complete_cluster_change(context):
    complete_change(context.trans, context.cid, context.rev)


@when('I lock future cluster')
def step_lock_future_cluster(context):
    context.actual_rev, context.next_rev = lock_future_cluster(context.trans, context.cid)


@when('I complete future cluster change')
def step_complete_future_cluster_change(context):
    complete_future_change(context.trans, context.cid, context.actual_rev, context.next_rev)


def apply_cluster_change(context, prefix=None):
    cid_var = prefix + '_cid' if prefix else 'cid'
    cid = getattr(context, cid_var)

    trans = connect(context.dsn, application_name='Apply cluster change')
    context.rev = lock_cluster(trans, cid)

    context.execute_error = None
    context.rows = None
    query, query_vars = make_query_from_context(context)
    try:
        context.rows = execute_query_in_transaction(trans, query, query_vars).fetch()
        complete_change(trans, cid, context.rev)
        trans.commit()
    except DatabaseError as exc:
        logging.warning('Query error %s', exc)
        context.execute_error = exc
        trans.rollback()
    finally:
        trans.close()


@when('I execute cluster change')
@when('I execute "{prefix:w}" cluster change')
def step_exec_cluster_change(context, prefix=None):
    apply_cluster_change(context, prefix)


@given('successfully executed cluster change')
def step_exec_cluster_change_ok(context):
    apply_cluster_change(context)
    assert_query_success(context)


@given('successfully executed query')
@when('I successfully execute query')
def step_success_query(context):
    query, query_vars = make_query_from_context(context)
    with execute_query(context.dsn, query, query_vars):
        pass


@given('successfully executed script')
@when('I successfully execute script')
def step_success_script(context):
    """
    execute script
    """
    query, query_vars = make_query_from_context(context)
    # don't use helpers, cause them remove endlines,
    # but them required in DO $$ ... END $$
    conn = connect(context.dsn, autocommit=True)
    try:
        cur = conn.cursor()
        cur.execute(query, query_vars)
    finally:
        conn.close()


def assert_query_success(context):
    if context.execute_error is not None:
        raise context.execute_error


@then('it success')
def step_check_query_success(context):
    assert_query_success(context)


def get_template_vars(text):
    """
    Get variables from template
    """
    env = jinja2.Environment()
    ast = env.parse(text)
    return jinja2.meta.find_undeclared_variables(ast)


def load_sls(context):
    """
    render context.text as jinja template and load it as yaml
    """
    variables = {}
    for var_name in get_template_vars(context.text):
        if var_name not in context:
            raise RuntimeError(f"Not variable '{var_name}' exists in context")
        variables[var_name] = getattr(context, var_name)
    template = jinja2.Template(context.text)
    rendred_text = template.render(**variables)
    return yaml.safe_load(rendred_text)


@given('expected row "{row_name:w}"')
def load_row_from_context(context, row_name):
    if 'expected_rows' not in context:
        context.expected_rows = {}
    context.expected_rows[row_name] = load_sls(context)


@then('it returns')
def step_check_query_without_errors(context):
    """
    Check that error is None and rows matches expected rows
    """
    expected_rows = load_sls(context)
    assert_query_success(context)
    assert_that([list(r) for r in context.rows], equal_to(expected_rows))


@then('It returns one row matches "{row_name:w}"')
def step_check_result_match_context(context, row_name):
    assert_query_success(context)

    assert_that(context.rows, has_length(1))
    assert_that(context.rows[0], has_properties(**context.expected_rows[row_name]))


@then('It returns nothing')
def step_check_query_return_no_rows(context):
    assert_query_success(context)
    assert_that(context.rows, is_(empty()))


class VerboseIsEqual(IsEqual):  # pylint: disable=too-few-public-methods
    """
    IsEqual and print diffs for text values
    """

    def __init__(self, equals):
        super().__init__(equals)
        self._called_on = None

    def _matches(self, item):
        self._called_on = item
        return super()._matches(item)

    def describe_to(self, description):
        if not isinstance(self.object, str) or not isinstance(self._called_on, str):
            super().describe_to(description)
            return

        description.append_description_of(self.object)
        description.append_text('\n')
        for line in difflib.ndiff(self.object.splitlines(), self._called_on.splitlines()):
            description.append_text(line + '\n')


def make_dict_matcher(dict_value):
    if not isinstance(dict_value, Mapping):
        return dict_value
    entries = {}
    for name, value in dict_value.items():
        entries[name] = make_dict_matcher(value)
    return all_of(has_entries(**entries), contains_inanyorder(*dict_value.keys()))


def make_row_matcher(row_dict):
    """
    Create matcher from dict
    """
    name2matcher = dict()
    for name, value in row_dict.items():
        matcher = make_dict_matcher(value)
        name2matcher[name] = matcher
    return has_properties(**name2matcher)


@then('it returns "{count:d}" rows matches')
def step_check_query_return_rows(context, count):
    assert_query_success(context)

    assert_that(context.rows, has_length(count))
    expected = load_sls(context)

    satisfy_matcher = contains_inanyorder(*(make_row_matcher(r) for r in expected))
    assert_that(context.rows, satisfy_matcher)


@then('it returns one row matches')
def step_check_query_return_one_row(context, count=1):
    assert_query_success(context)

    assert_that(context.rows, has_length(count))
    for expected_key, expected_value in load_sls(context).items():
        assert_that(context.rows[0], has_property(expected_key))
        assert_that(getattr(context.rows[0], expected_key), VerboseIsEqual(expected_value))


@then('it fail with error "{error_msg}"')
@then('it fail with error matches "{error_re}"')
def step_check_query_error(context, error_msg=None, error_re=None):
    assert_that(
        context.execute_error,
        has_property('pgerror', contains_string(error_msg) if error_msg else matches_regexp(error_re)),
    )


GET_STATUS_Q = verbose_query('SELECT status FROM dbaas.clusters WHERE cid = :cid')


@then('cluster in "{status}" status')
def step_check_cluster_in_status(context, status):
    cid = getattr(context, 'cid')
    with execute_query(context.dsn, GET_STATUS_Q, {'cid': cid}) as res:
        for row in res.fetch():
            if row[0] != status:
                raise AssertionError(f'expecting cluster in {status} status, got = {row[0]}')
            return
    raise AssertionError(f'cluster with {cid=} not found')

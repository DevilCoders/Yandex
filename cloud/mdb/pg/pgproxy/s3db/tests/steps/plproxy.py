# coding: utf-8

import helpers
from behave import then, when, given


@then(u'we get cluster partitions for {cluster_name}')
def step_get_cluster_partitions(context, cluster_name):
    context.result = context.connect.get_func(
        'plproxy.get_cluster_partitions',
        i_cluster_name=cluster_name
    )
    helpers.assert_no_errcode(context)
    helpers.assert_compare_objects_list(context)


@when(u'we set priority {priority} to host {host_name}')
def step_set_priority_to_connection(context, priority, host_name):
    context.result = context.connect.query("""
        UPDATE plproxy.priorities SET priority=%(priority)s WHERE host_id IN (SELECT host_id FROM plproxy.hosts WHERE host_name= %(host_name)s)
    """, priority=priority, host_name=host_name)
    helpers.assert_no_errcode(context)


@given(u'current DC is {dc}')
def step_set_current_dc(context, dc):
    result = context.connect.get("SELECT dc FROM plproxy.my_dc LIMIT 1")[0]
    if result:
        if result[0]['dc'] != dc:
            context.result = context.connect.query("UPDATE plproxy.my_dc SET dc = %(dc)s", dc=dc)
            helpers.assert_no_errcode(context)
    else:
        context.result = context.connect.query("INSERT INTO plproxy.my_dc (dc) VALUES (%(dc)s)", dc=dc)
        helpers.assert_no_errcode(context)


@given(u'default priorities')
def step_set_default_priorities(context):
    context.result = context.connect.query("""
        UPDATE plproxy.priorities p SET priority=h.base_prio FROM plproxy.hosts h WHERE p.host_id=h.host_id
    """)
    helpers.assert_no_errcode(context)

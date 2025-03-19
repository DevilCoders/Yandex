#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
    Steps to make features shorter
"""


@when(u'we issue heartbeat for {fqdn:FQDN} dom0')
def step_issue_heartbeat(context, fqdn):
    """
    Helper for shorter feature files
    """
    geo = fqdn[:3]
    context.execute_steps(
        u'''
        When we issue "POST /api/v2/dom0/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                geo: {geo}
                switch: {geo}1-s1
                cpu_cores: 56
                memory: 269157273600
                ssd_space: 7542075424768
                sata_space: 0
                max_io: 1468006400
                net_speed: 1310720000
                heartbeat: 1
                generation: 2
                disks: []
            """
        Then we get response with code 200
    '''.format(
            fqdn=fqdn, geo=geo
        )
    )


def wrap_str_for_yaml(text):
    """
    wrap string with quotes
    """
    if not text.startswith("'"):
        return "'" + text + "'"
    return text


@when(u'we launch container {fqdn:FQDN}')
@when(u'we launch container {fqdn:FQDN} in cluster {cluster:Name}')
@when(u'we launch container {fqdn:FQDN} with {cpu:CPU} cores')
def step_launch_container(context, fqdn, cluster='ffffffff-ffff-ffff-ffff-ffffffffffff', cpu='1'):
    """
    Helper to launch container
    """
    geo = fqdn[:3]
    cluster = wrap_str_for_yaml(cluster)
    context.execute_steps(
        u'''
        When we issue "PUT /api/v2/containers/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                project: mdb
                cluster: {cluster}
                geo: {geo}
                bootstrap_cmd: /usr/local/yandex/porto/mdb_bionic.sh {fqdn}
                cpu_guarantee: {cpu}
                cpu_limit: {cpu}
                memory_guarantee: 4 GB
                memory_limit: 4 GB
                net_guarantee: 16 MB
                net_limit: 16 MB
                io_limit: 100 MB
                extra_properties:
                    project_id: '0:1589'
                volumes:
                    - backend: native
                      path: /
                      dom0_path: /data/{fqdn}/rootfs
                      space_limit: 10 GB
                    - backend: native
                      path: /var/lib/postgresql
                      dom0_path: /data/{fqdn}/data
                      space_limit: 100 GB
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
            """
    '''.format(
            fqdn=fqdn, geo=geo, cluster=cluster, cpu=cpu
        )
    )


@when(u'we double CPU for container {fqdn:FQDN}')
def step_double_cpu_for_container(context, fqdn):
    """
    Helper to update CPU guarantee/limit for container
    """
    context.execute_steps(
        u'''
         When we issue "GET /api/v2/containers/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
    '''.format(
            fqdn=fqdn
        )
    )
    options = context.request.json()
    guarantee = options['cpu_guarantee'] * 2 if options['cpu_guarantee'] is not None else options['cpu_limit'] * 2
    limit = options['cpu_limit'] * 2

    context.execute_steps(
        u'''
         When we issue "POST /api/v2/containers/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            body:
                cpu_guarantee: '{guarantee}'
                cpu_limit: '{limit}'
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
            """
    '''.format(
            fqdn=fqdn, guarantee=guarantee, limit=limit
        )
    )


@when(u'we delete container {fqdn:FQDN}')
def step_delete_container(context, fqdn):
    """
    Helper to delete container
    """
    context.execute_steps(
        u'''
         When we issue "DELETE /api/v2/containers/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
                Accept: application/json
                Content-Type: application/json
            """
         Then we get response with code 200
          And body contains:
            """
            deploy:
              deploy_id: test-jid
            """
    '''.format(
            fqdn=fqdn
        )
    )


def indent_text(text, spaces_count=30):
    """
    Intent text with spaces
    """
    spaces = u' ' * spaces_count
    lines = [spaces + l for l in text.split(u'\n')]
    return u'\n'.join(lines)


@then(u'container info for {fqdn:FQDN} contains')
def step_get_container_info(context, fqdn):
    """
    Helper to get container info
    """
    # reindent text, cause
    # Gherkin should be formatted
    # * step need yaml and it should be formatter
    contains = indent_text(context.text)
    context.execute_steps(
        u'''
         When we issue "GET /api/v2/containers/{fqdn}" with:
            """
            timeout: 5
            headers:
                Authorization: OAuth 11111111-1111-1111-1111-111111111111
            """
         Then we get response with code 200
          And body contains:
            """
{contains}
            """
    '''.format(
            fqdn=fqdn, contains=contains
        )
    )

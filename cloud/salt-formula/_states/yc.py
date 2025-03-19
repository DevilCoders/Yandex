import json
import logging
import os
import re

try:
    import collections.abc as collections_abc
except ImportError:
    import collections as collections_abc

log = logging.getLogger(__name__)


def cloud(name, policies=None):
    _ = _get_env(cloud=name, folder=None)['YC_CLOUD']
    return {
        'name': name,
        'changes': {name: {'old': '', 'new': 'present'}},
        'result': True,
        'comment': 'Cloud {} created'.format(name)
    }


def folder(name, public_roles=(), **kwargs):
    if "id" in kwargs:
        env = _get_env(kwargs.get('cloud'), None)
        folder_id = kwargs["id"]
        params = [
            ("name", name),
            ("folder-id", folder_id),
        ]
        _get_or_create("folder", name, params, env)
    else:
        env = _get_env(kwargs.get('cloud'), name)
        folder_id = env['YC_FOLDER']
    ret = _create_access_bindings(env, "allAuthenticatedUsers", public_roles,
                                  folder=folder_id, subject_type='system')
    return ret


def image(name, url_var, default_url, timeout_in_sec=780, description="", **kwargs):
    uri = os.environ.get(url_var, default_url)
    params = [
        ('name', name),
        ('uri', uri),
        ('wait-timeout', timeout_in_sec),
        ('description', description),
    ]

    return _resource('image', name, params, **kwargs)


def pooling(name, image, zone_id, count, timeout_per_disk=300, type_id=None, **kwargs):
    env = _get_env(kwargs.get('cloud'), kwargs.get('folder'))
    image_id = _get_resource('image', image, env, 'id')['result']
    if type_id is None:
        type_ids = ["network-hdd", "network-nvme"]
    else:
        type_ids = [type_id]
    if not image_id:
        return {
            'name': name,
            'changes': {},
            'result': False,
            'comment': 'Image "{}" not found'.format(image),
        }

    created = {}
    for type_id in type_ids:
        cmd = "yc-cli disk-pooling show --image {} --zone-id {} --type-id {} -f json".format(
            image_id, zone_id, type_id)
        cmd_ret = __states__['cmd.run'](name=cmd, env=env)
        if not cmd_ret['result']:
            raise Exception('Failed to get disk pooling for image {} in zone {} of type {}: {}'.format(
                image_id, zone_id, type_id, cmd_ret['changes']['stderr']))

        response = json.loads(cmd_ret['changes']['stdout'])
        current_count = (response[0]['available'] + response[0]['creating']) if response else 0
        if current_count >= count:
            continue

        cmd = "yc-cli disk-pooling update --image {} --zone-id {} --type-id {} --disk-count {} --wait-timeout {}".format(
            image_id, zone_id, type_id, count - current_count, timeout_per_disk * (count - current_count))
        cmd_ret = __states__['cmd.run'](name=cmd, env=env)
        if not cmd_ret['result']:
            raise Exception('Failed to update disk pooling for image {} in zone {} of type {}: {}'.format(
                image_id, zone_id, type_id, cmd_ret['changes']['stderr']))

        created[type_id] = count - current_count

    if created:
        return {
            'name': name,
            'changes': {name: {'old': '', 'new': 'present'}},
            'result': True,
            'comment': 'Number of disks added to pool(s): {}'.format(created),
        }
    else:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': 'Pooling already created: no disks were added',
        }


def instance_group(name, max_instances_per_node, **kwargs):
    params = [
        ('name', name),
        ('max-instances-per-node', max_instances_per_node),
    ]
    return _resource('instance-group', name, params, **kwargs)


def placement_group(name, max_instances_per_node, max_instances_per_fault_domain, strict, **kwargs):
    params = [
        ('name', name),
        ('strict', strict),
    ]
    if max_instances_per_node is not None:
        params.append(('max-instances-per-node', max_instances_per_node))
    if max_instances_per_fault_domain is not None:
        params.append(('max-instances-per-fault-domain', max_instances_per_fault_domain))

    return _resource('placement-group', name, params, **kwargs)


def user(login, roles=(), root_roles=(), **kwargs):
    env = _get_env(kwargs.get('cloud'), kwargs.get('folder'))
    ret, user_id = _get_or_create('user-account', login, [('login', login)], env)
    if not ret['result']:
        return ret
    if roles:
        ret = _create_access_bindings(env, user_id, roles, cloud=env['YC_CLOUD'])
        if not ret['result']:
            return ret
    if root_roles:
        ret = _create_root_bindings(env, user_id, root_roles)
    return ret


def _make_network(name, **kwargs):
    params = [('name', name),]
    return _resource('network', name, params, **kwargs)


def network(name, **kwargs):
    return _make_network(name, **kwargs)


def _fip_bucket_get_id(name, ip_version, env):
    """ gets fip-bucket id by its name + ip_version, returns None if not found """
    cmd = "yc-cli fip-bucket list -f json -c name -c ip_version -c id"
    cmd_ret = __states__['cmd.run'](name=cmd, env=env)
    if not cmd_ret['result']:
        raise Exception('Failed to list fip-buckets: {}'.format(cmd_ret['changes']['stderr']))
    response = json.loads(cmd_ret['changes']['stdout'])
    for fb in response:
        if fb['name'] == name and (fb['ip_version'] or 'ipv4') == ip_version:
            return fb['id']
    return None


def fip_bucket(name, flavor, scope, cidrs, import_rts, export_rts, ip_version='ipv4', **kwargs):
    params = [
        ('flavor', flavor),
        ('scope', scope),
        ('ip-version', ip_version),
    ]
    for cidr in cidrs:
        params.append(('cidrs', cidr))
    for rt in import_rts:
        params.append(('import-rts', rt))
    for rt in export_rts:
        params.append(('export-rts', rt))

    env = _get_env(cloud=None, folder=None)
    fb_id = _fip_bucket_get_id(name, ip_version, env)
    if fb_id:
        # get existing by its id
        return _get_or_create('fip-bucket', fb_id, params, env, create=False)[0]
    else:
        return _create('fip-bucket', name, params, env)[0]


def subnet(name, network, zone_id, v4_cidr_block=None, v6_cidr_block=None, extra_params=None, auto_create_network=False, **kwargs):
    if auto_create_network:
        ret = _make_network(network, **kwargs)
        if not ret['result']:
            return ret

    params = [
        ('name', name),
        ('network', network),
        ('zone-id', zone_id),
    ]

    if v4_cidr_block:
        params.append(('v4-cidr-block', v4_cidr_block))

    if v6_cidr_block:
        params.append(('v6-cidr-block', v6_cidr_block))

    # Hack to provision v6-only networks.
    if not v4_cidr_block:
        if not extra_params:
            extra_params = {}
        extra_params['next_vdns'] = __pillar__['dns_forwarders'][__pillar__['dns']['forward_dns']['v6only']]

    if extra_params:
        extra_params_str = _dict_to_complex_argument(extra_params)
        params.append(('extra-params', extra_params_str))

    return _resource('subnet', name, params, **kwargs)


def quota(name, limit, **kwargs):
    ret = {
        'name': limit,
        'changes': {},
        'result': False,
        'comment': ''
    }
    env = _get_env(kwargs.get('cloud'), kwargs.get('folder'))
    cmd = "yc-cli quota set --name {name} --limit {limit} --target-cloud {cloud_id}".format(name=name, limit=limit, cloud_id=env['YC_CLOUD'])
    cmd_ret = __states__['cmd.run'](name=cmd, env=env)
    if cmd_ret['result']:
        ret['result'] = True
        ret['comment'] = 'quota "{}" was set to {}'.format(name, limit)
        ret['changes'].update({limit: {'old': '', 'new': 'present'}})
    else:
        ret['comment'] = 'Failed to set quota "{}" to {}'.format(name, limit)
        ret['changes'].update(cmd_ret['changes'])

    return ret


def _resource(typ, name, params, cloud=None, folder=None):
    """Either get or create the requested resource using the cli."""

    env = _get_env(cloud, folder)
    return _get_or_create(typ, name, params, env)[0]


def _get_grains_cloud_id_folder_id(cloud_name, folder_name):
    """Get cloud_id and folder_id as stored in grains (CLOUD-15452)"""

    cloud_id = __salt__['grains.get']('cluster_map:clouds:{}:id'.format(cloud_name), None)
    folder_id = __salt__['grains.get']('cluster_map:clouds:{}:folders:{}:id'.format(cloud_name, folder_name), None)

    return cloud_id, folder_id


def _get_env(cloud, folder):
    env = {'REQUESTS_CA_BUNDLE': '/etc/ssl/certs/ca-certificates.crt'}
    api_url = os.environ.get('YC_API_URL', os.environ.get('YC_BASE_URL'))
    if api_url:
        env['YC_API_URL'] = api_url

    grains_cloud_id, grains_folder_id = _get_grains_cloud_id_folder_id(cloud, folder)

    if cloud:

        owner_login = __pillar__['identity']['default_owner']
        if grains_cloud_id:
            env['YC_CLOUD'] = cloud_id = grains_cloud_id
        else:
            ret, cloud_id = _get_or_create('cloud', cloud, [('name', cloud), ('owner-login', owner_login)], env)
            if not cloud_id:
                raise Exception('Error creating cloud: %r' % ret)
            env['YC_CLOUD'] = cloud_id

        # Assign internal.computeadmin role to the owner to be able to use compute internal API
        compute_internal_role = 'internal.computeadmin'
        ret, user_id = _get_or_create('user-account', owner_login, [('login', owner_login)], env)
        if not user_id:
            raise Exception('Error getting %s user_id: %r' % (owner_login, ret))
        ret = _create_access_bindings(env, user_id, [compute_internal_role], cloud=cloud_id)
        if not ret['result']:
            raise Exception('Error setting compute internal roles for %s : %r' % (owner_login, ret))

    if folder:
        if grains_folder_id:
            env['YC_FOLDER'] = grains_folder_id
        else:
            ret, folder_id = _get_or_create('folder', folder, [('name', folder)], env)
            if not folder_id:
                raise Exception('Error creating folder: %r' % ret)
            env['YC_FOLDER'] = folder_id
    return env


def _dict_to_complex_argument(data):
    """
    Format dict as complex argument for yc-cli

    >>> _dict_to_complex_argument({'a': [1, 2], 'b': {'c': 'd'}})
    'a[0]=1,a[1]=2,b.c=d'
    >>> _dict_to_complex_argument({'a': [{'c': 'd'}, [1, 2]]})
    'a[0].c=d,a[1][0]=1,a[1][1]=2'
    """
    params = []

    def _build(prefix, value):
        if isinstance(value, collections_abc.Mapping):
            for k, v in value.items():
                _build('{}.{}'.format(prefix, k), v)
        elif isinstance(value, collections_abc.Iterable) and not isinstance(value, str):
            for i, v in enumerate(value):
                _build('{}[{}]'.format(prefix, i), v)
        else:
            params.append('{}={}'.format(prefix, value))

    for k, v in data.items():
        _build(k, v)

    return ','.join(params)


def _build_param(key, value):
    if value is True:
        return '--{}'.format(key)
    if value is False:
        return ''
    return '--{} "{}"'.format(key, value)


def _create(typ, name, params, env):
    ret = {
        'name': name,
        'changes': {},
        'result': False,
        'comment': '',
    }
    col = None
    params_str = ' '.join(_build_param(k, v) for k, v in params)
    create_cmd = 'yc-cli {} create -f value -c id {}'.format(typ, params_str)
    cmd_ret = __states__['cmd.run'](name=create_cmd, env=env)
    if cmd_ret['result']:
        col = cmd_ret['changes']['stdout']
        ret['result'] = True
        ret['comment'] = '{} "{}" created'.format(typ.capitalize(), name)
        ret['changes'].update({name: {'old': '', 'new': 'present'}})
    else:
        ret['comment'] = 'Failed to create {} "{}"'.format(typ, name)
        ret['changes'].update(cmd_ret['changes'])
    return ret, col


def _get_or_create(typ, name, params, env, column='id', create=True):
    ret = {
        'name': name,
        'changes': {},
        'result': False,
        'comment': '',
    }
    col = None
    state = _get_resource(typ, name, env, column)
    if state['success'] and state['result']:
        col = state['result']
        ret['comment'] = '{} "{}" was created'.format(typ.capitalize(), name)
        ret['result'] = True
    elif create and state['success'] and not state['result']:
        return _create(typ, name, params, env)
    else:
        ret['comment'] = 'Failed to get resource {} "{}"'.format(typ, name)
        ret['changes'] = state['result']
    return ret, col


def _get_resource(typ, name, env, column):
    show_cmd = 'yc-cli {} show -f value -c {} "{}"'.format(typ, column, name)
    cmd_ret = __states__['cmd.run'](name=show_cmd, env=env, output_loglevel='quiet')
    entity_not_found = re.search(r'Entity ".*?" not found', cmd_ret['changes']['stderr'])
    out = cmd_ret['changes']['stdout'] if cmd_ret['result'] else cmd_ret['changes']['stderr']
    return {
        'success': cmd_ret['result'] or entity_not_found,
        'result': None if not cmd_ret['result'] and entity_not_found else out
    }


def _create_access_bindings(env, subject_id, roles, cloud=None, folder=None, subject_type="userAccount"):
    if cloud is not None:
        cloud_id = _get_resource("cloud", cloud, env, "id")["result"]
        target_param = "--cloud-id {}".format(cloud_id)
    elif folder is not None:
        folder_id = _get_resource("folder", folder, env, "id")["result"]
        target_param = "--folder-id {}".format(folder_id)
    else:
        raise Exception("Logical error: one of 'cloud' or 'folder' should be specified.")

    cmd = "yc-cli access-binding list {target} -f json".format(target=target_param)
    cmd_ret = __states__['cmd.run'](name=cmd, env=env)
    if not cmd_ret['result']:
        raise Exception('Failed to list access-binding: {}'.format(cmd_ret['changes']['stderr']))

    response = json.loads(cmd_ret['changes']['stdout'])
    current_roles = [entry["role_id"] for entry in response
                     if entry["subject"]["id"] == subject_id and entry["subject"]["type"] == subject_type]

    new_roles = set(roles) - set(current_roles)
    if not new_roles:
        return {
            'name': 'create-access-bindings',
            'changes': {},
            'result': True,
            'comment': 'Access bindings already exist'
        }

    binding_delta_template = "--binding-delta action=add,accessBinding.roleId={role}," \
                             "accessBinding.subject.type={subject_type},accessBinding.subject.id={subject_id}"
    binding_deltas = " ".join(
        binding_delta_template.format(role=role, subject_type=subject_type, subject_id=subject_id) for role in new_roles
    )

    cmd = "yc-cli access-binding update {target} {deltas}".format(target=target_param, deltas=binding_deltas)
    cmd_ret = __states__['cmd.run'](name=cmd, env=env)
    ret = {
        'name': 'create-access-bindings',
        'changes': {},
        'result': False,
        'comment': '',
    }
    if cmd_ret['result']:
        ret['result'] = True
        ret['comment'] = 'Access bindings created successfully'
        ret['changes'].update(cmd_ret['changes'])
    else:
        ret['comment'] = 'Failed to create access bindings, command failed: {}'.format(cmd)
        ret['changes'].update(cmd_ret['changes'])

    return ret


def _create_root_bindings(env, subject_id, roles, subject_type="userAccount"):
    """Root bindings allow actions in ALL clouds."""

    binding_delta_template = "--binding-delta action=add,accessBinding.roleId={role}," \
                             "accessBinding.subject.type={subject_type},accessBinding.subject.id={subject_id}"
    binding_deltas = " ".join(
        binding_delta_template.format(role=role, subject_type=subject_type, subject_id=subject_id) for role in roles
    )

    cmd = "yc-cli root-binding update {deltas}".format(deltas=binding_deltas)
    cmd_ret = __states__['cmd.run'](name=cmd, env=env)
    ret = {
        'name': 'create-root-access-bindings',
        'changes': {},
        'result': False,
        'comment': '',
    }
    if cmd_ret['result']:
        ret['result'] = True
        ret['comment'] = 'Root access bindings created successfully'
        ret['changes'].update(cmd_ret['changes'])
    else:
        ret['comment'] = 'Failed to create root access bindings, command failed: {}'.format(cmd)
        ret['changes'].update(cmd_ret['changes'])

    return ret

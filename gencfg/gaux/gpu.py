from core.igroups import CIPEntry


def device_number(group, instance):
    return group.extra_data.get(CIPEntry(instance.host, instance.port))


def device(group, instance):
    return '/dev/{}'.format(device_name(group, instance))


def device_name(group, instance):
    return 'nvidia{}'.format(device_number(group, instance))


def host_devices(group, instance):
    """
    Render json for nanny, e.g.
        [
          {'path': '/dev/nvidia5',
           'mode': 'rw'}
        ]
        or [], or None, if group doesn't have allocated GPUs
    """
    number = device_number(group, instance)
    if number is None:
        return None
    return [{'path': device(group, instance),
             'mode': 'rw'}]


def _gpu_required(group):
    return bool(group.card.reqs.instances.get('gpu'))

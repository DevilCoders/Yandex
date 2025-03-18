"""
    In case if both Exp and RemoteLog are defined, module RemoteLog should be under Exp
"""


def check_exp_and_remote_log_order(modules_as_dict):
    recurse_check_exp_and_remote_log_order(modules_as_dict, [])


def recurse_check_exp_and_remote_log_order(modules, path):
    try:
        exp_index = path.index('exp')
        remote_log_index = path.index('remote_log')

        if remote_log_index < exp_index:
            raise Exception("Wrong order of Exp and RemoteLog modules in path %s" % (
                            " -> ".join(map(lambda x: str(x), path))))
    except ValueError:  # did not find exp or remote_log, whichi is Ok
        pass

    if issubclass(modules.__class__, dict):
        for k, v in modules.iteritems():
            path.append(k)
            recurse_check_exp_and_remote_log_order(v, path)
            path.pop()
    elif isinstance(modules, (list, tuple)):
        for i, elem in enumerate(modules):
            path.append('[%s]' % i)
            recurse_check_exp_and_remote_log_order(elem, path)
            path.pop()

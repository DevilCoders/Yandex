from typing import Any, List, Union, Type


from importlib import import_module

_hooks = dict()
_connections = dict()


def import_string(dotted_path):
    """
    Import a dotted module path and return the attribute/class designated by the
    last name in the path. Raise ImportError if the import failed.
    """
    try:
        module_path, class_name = dotted_path.rsplit('.', 1)
    except ValueError:
        raise ImportError("{} doesn't look like a module path".format(dotted_path))

    module = import_module(module_path)

    try:
        return getattr(module, class_name)
    except AttributeError:
        raise ImportError('Module "{}" does not define a "{}" attribute/class'.format(
            module_path, class_name)
        )


def get_hook(conn_id: str):
    from cloud.dwh.lms.hooks.base_hook import BaseHook
    if not _hooks.get(conn_id):
        print(f"getting hook for conn_id={conn_id}")
        connection = BaseHook.get_connection(conn_id)
        _hooks[conn_id] = connection.get_hook()
    return _hooks[conn_id]


def get_cursor(conn_id: str):
    if not _connections.get(conn_id):
        print(f"getting cursor for conn_id={conn_id}")
        _connections[conn_id] = get_hook(conn_id).get_conn()
        print(f"got cursor for conn_id={conn_id}")
    return _connections[conn_id].cursor()


def check_type(var_name: str, var_value: Any, allowed_types: Union[Type[Any], List[Type[Any]]]):
    if not type(allowed_types) == list:
        allowed_types = [allowed_types]
    if type(var_value) not in allowed_types:
        raise ValueError(f"Invalid type for {var_name}: expected one of {allowed_types} instead got: {type(var_value)}")

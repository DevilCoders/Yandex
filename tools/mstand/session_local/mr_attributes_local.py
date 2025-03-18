import os
import yt.yson as yson


def _attributes_path(path):
    return "{}.meta".format(path)


def load_attributes(path):
    path_attr = _attributes_path(path)
    if not os.path.isfile(path_attr):
        return {}
    with open(path_attr, "rb") as f:
        attributes = yson.load(f)
    return yson.yson_to_json(attributes.get("attributes", {}))


def save_attributes(path, attributes):
    assert isinstance(attributes, dict)
    path_attr = _attributes_path(path)
    with open(path_attr, "wb") as f:
        yson.dump({"attributes": attributes}, f)


def set_attribute(path, name, value):
    attributes = load_attributes(path)
    attributes[name] = value
    save_attributes(path, attributes)


def get_attribute(path, name, default):
    attributes = load_attributes(path)
    return attributes.get(name, default)

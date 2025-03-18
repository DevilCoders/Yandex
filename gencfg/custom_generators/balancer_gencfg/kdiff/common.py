#!/skynet/python/bin/python

import collections
import copy
import hashlib

shared = {}


def to_seconds(v):
    if v.endswith("ms"):
        return float(v[:-2]) / 1000.0
    elif v.endswith("s"):
        return float(v[:-1])
    else:
        return float(v)


path = []


def traverse(node_name, json_node, func, func2):
    if func2:
        r = func2(node_name, json_node)
        if r:
            return r

    children_results = []

    path.append((node_name, json_node))
    if type(json_node) is dict:
        for child_name in sorted(json_node.keys()):
            res = traverse(child_name, json_node[child_name], func, func2)
            children_results.append((child_name, res))
    if type(json_node) is list:
        i = 0
        for list_node in json_node:
            child_name = node_name + "_" + str(i)
            res = traverse(child_name, list_node, func, func2)
            children_results.append((child_name, res))
            i += 1

    path.pop()
    if func:
        return func(path, node_name, json_node, children_results)
    else:
        return None


def fill_shared(json_node):
    def f(path, name, node, children_results):
        if name == "shared" and len(node) > 1:
            try:
                uuid = str(node["uuid"])
            except Exception:
                print name
                print node
                raise
            if uuid in shared:
                raise Exception("multiple shared definition")

            shared[uuid] = {}

            for k, v in node.iteritems():
                if k == "uuid":
                    continue
                shared[uuid][k] = v

    traverse("root", json_node, f, None)


def replace_shared(json_node):
    def f(node_name, node):
        if type(node) is dict and "shared" in node:
            uuid = str(node["shared"]["uuid"])
            del node["shared"]

            node.update(copy.deepcopy(shared[uuid]))

    traverse("root", json_node, None, f)


subtree_counter = collections.defaultdict(int)


def add_subtree_hash(json_node):

    def f(path, node_name, node, children_results):
        if type(node) is dict and "subtree_hash" in node:
            subtree_counter[node["subtree_hash"]] += 1
            return node["subtree_hash"]

        m = hashlib.md5()
        for cname, chash in sorted(children_results):
            m.update(cname)
            m.update(chash)

        m.update(node_name)
        if not children_results:
            m.update(repr(node))

        d = m.hexdigest()

        subtree_counter[d] += 1

        if type(node) is dict and children_results:
            node["subtree_hash"] = d

        return d

    traverse("root", json_node, f, None)

    def f2(path, name, node, children_results):
        if type(node) is dict and "subtree_hash" in node:
            node["subtree_hash_count"] = subtree_counter[node["subtree_hash"]]

            depth = 0
            if children_results:
                depth = max([_[1] for _ in children_results]) + 1

            node["subtree_depth"] = depth
            return depth
        else:
            return 0

    traverse("root", json_node, f2, None)


def fix_reports(json_node):
    def f(node_name, node):
        if node_name == "report":
            to_erase = {
                "events": {u"stats": u"report"},
                "disable_robotness": True,
                "disable_sslness": True,
                "just_storage": False
            }

            for key, value in to_erase.items():
                if key in node and node[key] == value:
                    del node[key]

            if "refers" in node:
                ids = node["refers"].split(",")
                if "uuid" in node:
                    ids.append(node["uuid"])

                node["uuid"] = ",".join(sorted(ids))

                del node["refers"]

        return None

    traverse("root", json_node, None, f)

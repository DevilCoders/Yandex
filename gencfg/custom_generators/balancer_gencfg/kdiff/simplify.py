#!/skynet/python/bin/python

import common
import json
import sys
import collections
import hashlib
import codecs
import argparse


_MOVED_OUT = "MOVED OUT"
_BALANCER_TYPES = ["rr", "weighted2", "active", "hashing", "dynamic"]


def collapse_backends(json_node):
    def f(path, node_name, node, children_results):
        if children_results and True in [_[1] for _ in children_results]:
            return True

        if node_name == "active" and path[-1][0] == "dynamic":
            return False

        if node_name in _BALANCER_TYPES and type(node) is dict:
            items = node.items()
            for k, v in items:
                has_proxy = False
                k_, v_ = k, v
                while type(v_) is dict:
                    if k_ == "proxy":
                        has_proxy = True
                        break

                    k_ = None

                    for k__, v__ in v_.items():
                        if type(v__) is dict:
                            k_, v_ = k__, v__
                            break

                    if not k_:
                        break

                if has_proxy:
                    del node[k]

            return True

        return False

    common.traverse("root", json_node, f, None)


def rewrite_balancer(json_node):
    def f(path, node_name, node, children_results):
        opts = {}
        if node_name == "proxy":
            proxy_opts = {}

            opts["backends"] = [
                "{}:{} [{}]".format(
                    node.get("host"),
                    node.get("port"),
                    node.get("cached_ip")
                ),
            ]

            for k, v in node.items():
                if k in ["cached_ip", "host", "port"]:
                    continue

                proxy_opts[k] = v

            opts["proxy_opts"] = proxy_opts
        elif node_name == "statistics":
            stat_opts = {}

            for k, v in node.items():
                if k == "proxy":
                    continue
                stat_opts[k] = v

            opts["stat_opts"] = stat_opts
        elif not node_name.startswith("balancer") and not node_name in _BALANCER_TYPES and not (len(path) > 0 and path[-1][0] in _BALANCER_TYPES):
            return {}

        for cname, v in children_results:
            if not v:
                continue

            for opts_key, opts_v in v.items():
                if opts_v is None:
                    continue
                elif opts_key not in opts:
                    opts[opts_key] = opts_v
                elif opts_key == "backends":
                    opts[opts_key].extend(opts_v)
                elif opts[opts_key] != opts_v:
                    print opts_key
                    print " ".join(_[0] for _ in path)
                    print opts[opts_key]
                    print opts_v
                    raise Exception("diff in opts, unsupported")

        if node_name.startswith("balancer"):
            for k, v in opts.items():
                node[k] = v

            if "backends" in node:
                backends_list = node["backends"]
                m = hashlib.md5()
                for b in sorted(backends_list):
                    m.update(b)

                node["backends"] = m.hexdigest()

            opts = {}

        return opts

    common.traverse("root", json_node, f, None)


min_subtree_depth = 3
common_nodes = {}
common_nodes_counter = collections.defaultdict(int)


def _subtree_hash_mult(node):
    return "{subtree_hash} x {count}".format(
        subtree_hash=node["subtree_hash"],
        count=node["subtree_hash_count"],
    )


def linearize(json_node, options):
    def f(path, node_name, node, children_results):
        if type(node) is dict and "subtree_hash_count" in node and node["subtree_hash_count"] > 1:
            if len(path) == 0:
                return

            # if "subtree_depth" not in node or node["subtree_depth"] <= min_subtree_depth:
            #     return

            if not type(path[-1][1]) is dict:
                return

            if node_name in ["proxy_opts", "stat_opts", "proxy", "statistics"]:
                return

            if path[-1][1]["subtree_hash_count"] == node["subtree_hash_count"]:
                return

            upper_report_name = ""
            ssl = "no_ssl"
            trusted_networks = "not_trusted"
            for path_item in reversed(path):
                if path_item[0] == "report" and not upper_report_name:
                    if "uuid" not in path_item[1] or path_item[1]["uuid"] == "antirobot":
                        continue
                    upper_report_name = path_item[1]["uuid"]

                if path_item[0] == "ssl_sni":
                    ssl = "ssl"
                if path_item[0] == "trusted_networks":
                    trusted_networks = "trusted"

            common_name = (node_name, _subtree_hash_mult(node))
            common_nodes_counter[node_name] += 1

            global common_nodes
            if common_name not in common_nodes:
                tmp = set()
                tmp.add(upper_report_name)
                node["upper_report_name"] = tmp

                tmp2 = set()
                tmp2.add(ssl)
                tmp2.add(trusted_networks)
                node["markers"] = tmp2
                node["counter"] = common_nodes_counter[node_name]
                common_nodes[common_name] = node
            else:
                common_nodes[common_name]["upper_report_name"].add(upper_report_name)
                common_nodes[common_name]["markers"].add(ssl)
                common_nodes[common_name]["markers"].add(trusted_networks)

            try:
                path[-1][1][node_name] = {
                    "subtree_hash": node["subtree_hash"],
                    "subtree_hash_count": node["subtree_hash_count"],
                    _MOVED_OUT: True,
                }
            except Exception:
                print path[-1][1]
                raise

    common.traverse("root", json_node, f, None)

    def cleanup(node_name, node):
        if not type(node) is dict:
            return

        for n in ["upper_report_name", "markers"]:
            if n in node and type(node[n] is set):
                node[n] = " ".join(sorted(node[n]))

        if "subtree_depth" in node:
            del node["subtree_depth"]
            del node["subtree_hash"]
            del node["subtree_hash_count"]

        if "subtree_hash_count" in node:
            if _MOVED_OUT in node:
                node[_MOVED_OUT] = node["subtree_hash"]

            if node["subtree_hash_count"] == 1:
                del node["subtree_hash"]
                node["UNIQUE"] = True
            else:
                node["subtree_hash"] = _subtree_hash_mult(node)

            del node["subtree_hash_count"]

            if _MOVED_OUT in node:
                del node["subtree_hash"]

    global common_nodes
    common.traverse("root", json_node, None, cleanup)
    common.traverse("common", common_nodes, None, cleanup)

    tmp = {}
    for name, node in common_nodes.items():
        module, subtree_hash = name

        markers = ""
        report_names = ""
        counter = ""

        if "markers" in node:
            markers = node["markers"]
            del node["markers"]

        if node["upper_report_name"]:
            report_names = node["upper_report_name"]
            del node["upper_report_name"]

        if "counter" in node:
            counter = node["counter"]
            del node["counter"]

        new_name = (module, markers, counter, report_names, subtree_hash)

        tmp[new_name] = node

    common_nodes = tmp


indent = "  "
depth = 0


def dump(node, f, hash_markers):
    global depth
    if type(node) is dict:
        children_dicts = []
        children_lists = []
        for key in sorted(node.keys()):
            v = node[key]
            if type(v) is dict:
                children_dicts.append((key, v))
            elif type(v) is list:
                children_lists.append((key, v))
            else:
                if key == _MOVED_OUT and hash_markers:
                    v = "@@HASH_LINK_BEGIN@" + v + "@@HASH_LINK_END@"
                print >>f, u"{}{}: {}".format(indent * depth, key, v)

        for key, v in children_dicts:
            print >>f, "{}{}: {{".format(indent * depth, key)
            depth += 1
            dump(v, f, hash_markers)
            depth -= 1
            print >>f, "{}}} -- {}".format(indent * depth, key.split()[0])

        for key, v in children_lists:
            print >>f, "{}{}: [".format(indent * depth, key)
            depth += 1
            dump(v, f, hash_markers)
            depth -= 1
            print >>f, "{}] -- {}".format(indent * depth, key.split()[0])

        return

    if type(node) is list:
        for v in node:
            depth += 1
            if type(v) is dict:
                print >>f, "{}{{".format(indent * depth)
                dump(v, f, hash_markers)
                print >>f, "{}}}".format(indent * depth)
            elif type(v) is list:
                print >>f, "{}[".format(indent * depth)
                dump(v, f, hash_markers)
                print >>f, "{}]".format(indent * depth)
            else:
                dump(v, f, hash_markers)
            depth -= 1

        return

    print >>f, "{}{}".format(indent * depth, node)


def _stderr(message):
    print >>sys.stderr, message


def main():
    parser = argparse.ArgumentParser(description="Simplify balancer config")
    parser.add_argument(
        "-i", "--input",
        type=str,
        default=None,
        help="Input config file (will use stdin if omitted)",
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        default=None,
        help="Output config file (will use stdout if omitted)",
    )
    parser.add_argument(
        "-m", "--hash-markers",
        action="store_true",
        default=False,
        help="Put hash markers around hash for anchors generation in diff [optional]",
    )

    options = parser.parse_args()

    if options.input:
        with open(options.input) as f:
            json_cfg = json.load(f)
    else:
        _stderr("Will read config from stdin")
        json_cfg = json.load(sys.stdin)

    rewrite_balancer(json_cfg)

    common.fill_shared(json_cfg)
    common.replace_shared(json_cfg)
    common.fix_reports(json_cfg)

    _stderr("calc subtree count")
    common.add_subtree_hash(json_cfg)

    _stderr("start collapse_backends")
    collapse_backends(json_cfg)

    # print json.dumps(json_cfg, indent=2, sort_keys=True)

    linearize(json_cfg, options)

    output_f = (
        codecs.open(options.output, 'w', encoding='utf8') if options.output else
        codecs.getwriter('utf8')(sys.stdout)
    )
    output_f.write("Common subtrees\n")

    lex_counters = []
    for x in range(0, 20000):
        lex_counters.append(str(x))
    lex_counters.sort()

    global depth
    for name in sorted(common_nodes.keys()):
        module, markers, counter, report_names, subtree_hash = name
        output_f.write(
            "{module} [{markers}] #{lex_counters} {hash_marker_begin}{subtree_hash}{hash_marker_end}: {{\n".format(
                module=module,
                markers=markers,
                lex_counters=lex_counters[int(counter)],
                subtree_hash=subtree_hash,
                hash_marker_begin="@@HASH_ANCHOR_BEGIN@" if options.hash_markers else "",
                hash_marker_end="@@HASH_ANCHOR_END@" if options.hash_markers else "",
            )
        )
        for report_name in sorted(report_names.split()):
            output_f.write("{} {}\n".format(indent * depth, report_name))

        depth += 1
        dump(common_nodes[name], output_f, options.hash_markers)
        depth -= 1
        output_f.write("}\n")

    # output_f.write(json.dumps(common_nodes, indent=2, sort_keys=True))
    output_f.write("\n\n")
    output_f.write("Config")
    # output_f.write(json.dumps(json_cfg, indent=2, sort_keys=True))
    dump(json_cfg, output_f, options.hash_markers)


if __name__ == "__main__":
    main()

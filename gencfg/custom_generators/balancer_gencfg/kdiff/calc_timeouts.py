#!/skynet/python/bin/python

import common
import json
import sys

path = []

def calc_timeout(name, json_node):
    sub_modules_timeouts = []
    path.append(name)
    if type(json_node) is dict:

        for child_name, child_node in json_node.items():
            if not type(child_node) in [dict, list]:
                continue

            if child_name == "shared":
                res = calc_timeout(child_name, shared[child_node["uuid"]])
            else:
                res = calc_timeout(child_name, child_node)
            sub_modules_timeouts.append((child_name, res))

    elif type(json_node) is list:
        for list_node in json_node:
            if type(list_node) is dict:
                for child_name, child_node in list_node.items():
                    if not type(child_node) in [dict, list]:
                        continue

                    if child_name == "shared":
                        res = calc_timeout(child_name, shared[child_node["uuid"]])
                    else:
                        res = calc_timeout(child_name, child_node)

                    sub_modules_timeouts.append((child_name, res))
            elif type(list_node) is list:
                print "ERROR! nested lists"
    else:
        pass
    path.pop()

    if name == "proxy":
        t = common.to_seconds(json_node["backend_timeout"])
    elif name in ["balancer2", "balancer"]:
        t = 0.0
        for child_name, timeout in sub_modules_timeouts:
            if child_name == "on_error":
                t += timeout
            else:
                t += timeout * int(json_node["attempts"])
    elif name == "request_replier":
        t = 0.0
        for child_name, timeout in sub_modules_timeouts:
            if child_name != "sink":
                t += timeout
    elif name in ["rr", "regexp", "regexp_path", "hashing", "weighted", "weighted2", "active"]:
        if sub_modules_timeouts:
            t = max([x[1] for x in sub_modules_timeouts])
        else:
            t = 0
            print "-".join(path) + "-" + name, "EMPTY!"
    else:
        t = sum([x[1] for x in sub_modules_timeouts])

    if t > 0:
        print "-".join(path) + "-" + name, t, sub_modules_timeouts
    return t

def main():
    json_cfg = json.load(sys.stdin)


    common.fill_shared(json_cfg)
    common.replace_shared(json_cfg)
    common.fix_reports(json_cfg)

    for name, node in json_cfg.items():
        calc_timeout(name, node)

if __name__ == "__main__":
    main()

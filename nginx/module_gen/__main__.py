import os
import sys
import json

modules = []
for peerdir in sys.argv[1:]:
    try:
        module = os.path.join(peerdir, 'modules.json')
        with open(module, 'r') as f:
            diff = json.load(f)
            modules.extend(diff)
    except:
        sys.stderr.write('Error processing %s\n' % (module,))
        raise


# Range body, copy and not modified filters MUST be at the end of the list
def move_to_end(modules, module):
    if module in modules:
        modules.remove(module)
        modules.append(module)

move_to_end(modules, 'ngx_http_copy_filter_module')
move_to_end(modules, 'ngx_http_range_body_filter_module')
move_to_end(modules, 'ngx_http_not_modified_filter_module')
move_to_end(modules, 'ngx_http_slice_filter_module')


# Set order modules
result = []
for module in modules:
    if isinstance(module, list) and len(module) == 3:
        if module[0] in result:
            raise Exception('dublicated module "%s".\n' % (module,))
        else:
            result.append(module[0])
    else:
        if module in result:
            raise Exception('dublicated module "%s".\n' % (module,))
        else:
            result.append(module)


def move_to_previous(result, i, j):
    result.remove(i)
    result.insert(result.index(j), i)


def move_to_next(result, i, j):
    result.remove(i)
    result.insert(result.index(j) + 1, i)

ok = len(modules) > 0
last_sensitive_move = ''
for iter in range(len(modules)):
    ok = True
    for module in modules:
        if isinstance(module, list):
            if len(module) != 3:
                raise Exception('Error processing: len(list) != 3 in "%s".\n' % (module,))
            if not module[2] in result:
                raise Exception('Error processing: Unknown module "%s" in "%s".\n' % (module[2], module,))
            if module[1] == 'before':
                if result.index(module[0]) >= result.index(module[2]):
                    ok = False
                    last_sensitive_move = module
                    move_to_previous(result, module[0], module[2])
            elif module[1] == 'just_before':
                move_to_previous(result, module[0], module[2])
            elif module[1] == 'after':
                if result.index(module[0]) <= result.index(module[2]):
                    ok = False
                    last_sensitive_move = module
                    move_to_next(result, module[0], module[2])
            elif module[1] == 'right_after':
                move_to_next(result, module[0], module[2])
            else:
                raise Exception('Unknown order "%s" in "%s". Only before | after.\n' % (module[1], module,))
        elif not isinstance(module, str):
            raise Exception('Error processing: "%s" type:%s not str | list\n' % (module, type(module),))
    if ok:
        break

if not ok:
    raise Exception('Error processing: possible cycle "%s"\n' % (last_sensitive_move,))

modules = result


print("""
#include <ngx_config.h>
#include <ngx_core.h>
""")

for module in modules:
    print("extern ngx_module_t  {};".format(module))

print("ngx_module_t *ngx_modules[] = {")
print("\n".join(("&{},".format(module) for module in modules)))
print("NULL")
print("};")

print("char *ngx_module_names[] = {")
print("\n".join(("\"{}\",".format(module) for module in modules)))
print("NULL")
print("};")

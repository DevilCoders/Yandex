#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    GLOBAL = other1
    T = localhost, other2
    
    T t1:
    T t2:
    T t3:
    T t_cn: t2
    GLOBAL t4: t3
    T t5: t1 t_cn t4
}
EOF

# vim: set ts=4 sw=4 et:

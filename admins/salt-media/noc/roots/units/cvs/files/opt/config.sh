reals="{%- for host in salt.conductor.groups2hosts('nocdev-cvs') %}{{ host }}{{ "," if not loop.last else "" }} {%- endfor %}"
myhost=`hostname -f`

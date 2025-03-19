{%- for disk in disks %}
[{{ type }}test]
blocksize=4k
filename=/dev/{{ disk }}
rw=rand{{ type }}
direct=1
buffered=0
ioengine=libaio
iodepth=4
{% endfor %}

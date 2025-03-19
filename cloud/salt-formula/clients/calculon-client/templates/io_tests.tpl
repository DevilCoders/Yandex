[global]
random_generator=lfsr
direct=1
buffered=0
ioengine=libaio
iodepth=4
blocksize=4k
{%- for disk in disks %}
[{{ type }}test]
filename=/dev/{{ disk }}
rw=rand{{ type }}
{% endfor %}

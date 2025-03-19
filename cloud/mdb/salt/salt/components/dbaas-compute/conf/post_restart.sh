#!/bin/bash

set -x
export SYSTEMD_PAGER=''

if [ "$1" = "disk_resize" ]
then
{% if accumulator | default(False) %}
{%   if 'compute-post-restart-before-disk-resize' in accumulator %}
{%     for line in accumulator['compute-post-restart-before-disk-resize'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}
    BOOT_DEVICE="$(/bin/mount | /usr/bin/cut -f 1,3 -d ' ' | /bin/grep -E '\ /$' | /usr/bin/cut -d ' ' -f1 | /bin/sed 's/[0-9]*//g')"
    DATA_DEVICE="$(/bin/ls /dev/vd* | /bin/grep -v "${BOOT_DEVICE}" | /bin/sed 's/[0-9]*//g' | /usr/bin/sort -u)"
    /sbin/sgdisk -d 1 "${DATA_DEVICE}" || exit 1
    /sbin/sgdisk -N 1 "${DATA_DEVICE}" || exit 1
    partprobe "${DATA_DEVICE}" || exit 1
    resize2fs "${DATA_DEVICE}1" || exit 1
fi

{% if accumulator | default(False) %}
{%   if 'compute-post-restart' in accumulator %}
{%     for line in accumulator['compute-post-restart'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}

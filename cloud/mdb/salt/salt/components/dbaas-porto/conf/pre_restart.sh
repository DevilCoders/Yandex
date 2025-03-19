#!/bin/bash

set -x
export SYSTEMD_PAGER=''

{% if accumulator | default(False) %}
{%   if 'compute-pre-restart' in accumulator %}
{%     for line in accumulator['compute-pre-restart'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}

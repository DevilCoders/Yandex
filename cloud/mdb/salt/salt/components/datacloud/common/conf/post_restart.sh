#!/bin/bash

set -x
export SYSTEMD_PAGER=''

{% if accumulator | default(False) %}
{%   if 'compute-post-restart' in accumulator %}
{%     for line in accumulator['compute-post-restart'] %}
{{ line }}
{%     endfor %}
{%   endif %}
{% endif %}

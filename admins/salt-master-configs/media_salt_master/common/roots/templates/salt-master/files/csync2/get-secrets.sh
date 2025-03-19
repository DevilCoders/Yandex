#!/bin/bash

USER={{ user }}

mkdir -p /var/tmp/salt
git clone git@{{ csync2.repo }} --branch {{ csync2.branch|default("master") }} /var/tmp/salt/
cp /var/tmp/salt/{{ csync2.path }}/csync2* /etc/
rm -r /var/tmp/salt

#!/bin/bash

USER={{ user }}

mkdir -p /var/tmp/salt
git clone git@{{ ssh.key_repo }} --branch {{ ssh.key_repo_branch|default("master") }} /var/tmp/salt/
cp /var/tmp/salt/{{ ssh.key_path_in_repo }}/{{ ssh.key_name_in_repo }} /home/$USER/.ssh/
rm -r /var/tmp/salt

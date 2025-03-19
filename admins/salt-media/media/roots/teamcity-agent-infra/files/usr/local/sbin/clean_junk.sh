#!/usr/bin/env bash

find /home/teamcity/.ya/tools -type f -mtime +14 -delete
find /home/teamcity/.gradle/daemon -type f -mtime +14 -delete
find /home/teamcity/.cache -type f -mtime +14 ! -path "/home/teamcity/.cache/node-gyp/*" ! -path "/home/teamcity/.cache/yarn/*" -prune -print0 | xargs -0 rm -rf
find /home/teamcity/.yarn-cache -type f -mtime +14 -delete
find /home/teamcity/.ichwill/sandbox-cache -type f -mtime +14 -delete
find /var/lib/teamcity/BuildAgents/$(hostname)/work -type f -mtime +14 -delete
sudo -u teamcity /usr/bin/ya gc cache --size-limit=50Gib
docker system prune -a -f --volumes

# EOF


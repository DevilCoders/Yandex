# yandex-media-cassandra-repair-tool
Tool for repair cassandra

made in https://github.yandex-team.ru/salt-media/music/tree/master/roots/cass/repair


```
usage: tool.py [-h] (-r | -b | -m | --rotate) [-q]

Repair a range of token on a host or rebuild a host.

optional arguments:
  -h, --help     show this help message and exit
  -r, --repair   Repair the host
  -b, --rebuild  Rebuild the host
  -m, --monrun   Display a monrun check
  --rotate       Rotate monitoring log
  -q, --quite    Don't output to stdin
```

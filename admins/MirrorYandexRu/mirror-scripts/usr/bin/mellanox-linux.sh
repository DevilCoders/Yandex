#!/bin/bash
cd /mirror/
wget -r -l 0 -N -c --no-parent --reject index.html* https://linux.mellanox.com/public/repo/


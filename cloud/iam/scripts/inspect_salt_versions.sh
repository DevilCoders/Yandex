#!/usr/bin/env bash
set -e

hosts=$@

if [[ $hosts = "" ]] ; then
  echo "Single argument is required - <host> to pass to pssh, typically 'C@cloud...' or 'x@....yandex.net'"
  exit 1
fi

# collect data in format '# host role version'
PSSH_OUT=$(pssh run -p 5 'sudo grep yc-salt-formula /srv/yc/ci/salt-formulae/*/release.sls | sed "s/.srv.yc.ci.salt-formulae./# $(hostname) /" | sed "s/\/release.sls:    yc-salt-formula://"' $hosts)

# using '#' as an indicator, remove all extra output produced by pssh,
# then remove '#' and rearrange columns for meaningful sorting - role, host, version
DATA=$(echo "$PSSH_OUT" | grep -E '^#.*' | awk -F' ' '{print $3 "\t" $2 "\t" $4}' | sort)

# highlight salt version that distinct from others (actually - from the previous row)
echo "$DATA" | awk -F'\t' '{
  if (prev1 && $1!=prev1)
    print "";
  if ($1==prev1 && $3 != prev3)
   style="\033[0;31m" ;
  else
   style="" ;
 print $1 " " $2 " " style $3 "\033[0m ";
 prev3=$3;
 prev1=$1
}'

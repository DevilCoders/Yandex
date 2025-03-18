#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

cd $base_path || exit 1

./dump_hostsdata.sh > ALL_HOSTS.txt # no background to avoid races after svn update

./utils/common/show_machine_types.py --no-background -ns ALL > ALL_MASTER_GROUPS.txt &

./dump_hostsdata.sh -g ALL_UNWORKING_MAINTENANCE > ALL_UNWORKING_MAINTENANCE.txt &
./dump_hostsdata.sh -g ALL_YP_HOSTS > ALL_YP_HOSTS.txt &
./dump_hostsdata.sh -g ALL_QLOUD_HOSTS > ALL_QLOUD_HOSTS.txt &
./dump_hostsdata.sh -g MSK_RESERVED,VLA_RESERVED,SAS_RESERVED,MAN_RESERVED,UNKNOWN_RESERVED > GENCFG_RESERVED.txt &
./dump_hostsdata.sh -g RESERVE_SELECTED > RESERVE_SELECTED.txt &
./dump_hostsdata.sh -g RESERVE_GPU > RESERVE_GPU.txt &
./dump_hostsdata.sh -g RESERVE_BAD > RESERVE_BAD.txt &
./dump_hostsdata.sh -g RESERVE_TRANSFER > RESERVE_TRANSFER.txt &
./dump_hostsdata.sh -g RESERVE_UPDATE > RESERVE_UPDATE.txt &
./dump_hostsdata.sh -g RESERVE_NEW_HOSTS > RESERVE_NEW_HOSTS.txt &

./utils/common/show_power.py -g ".*DYNAMIC_YP.*" > DYNAMIC_YP.txt &
./optimizers/dynamic/main.py -a get_stats -m  ALL_DYNAMIC > ALL_DYNAMIC_free.txt &

./utils/common/show_power.py --hosts-power -d -g ALL_UNWORKING_MAINTENANCE,MSK_RESERVED,VLA_RESERVED,SAS_RESERVED,MAN_RESERVED,RESERVE_SELECTED,RESERVE_BAD,RESERVE_TRANSFER,RESERVE_UPDATE,RESERVE_GPU,RESERVE_NEW_HOSTS > RESERVES_STAT.txt &

./dump_hostsdata.sh -f "lambda x: set(x.walle_tags).intersection('g:yp-iss-sas g:yp-iss-man g:yp-iss-vla g:yp-iss-iva g:yp-iss-myt'.split())" > YP_DEFAULT.txt &
./dump_hostsdata.sh -f "lambda x: set(x.walle_tags).intersection('g:yp-iss-sas-dev g:yp-iss-man-dev g:yp-iss-vla-dev g:yp-iss-iva-dev g:yp-iss-myt-dev'.split())" > YP_DEV.txt &

wait

echo ALL_DYNAMIC > STAT.txt
cat ALL_DYNAMIC_free.txt >> STAT.txt
echo DYNAMIC_YP >> STAT.txt
cat DYNAMIC_YP.txt >> STAT.txt
echo >> STAT.txt
echo RESERVES_STAT >> STAT.txt
cat RESERVES_STAT.txt >> STAT.txt

cat STAT.txt

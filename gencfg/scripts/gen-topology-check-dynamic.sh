#!/usr/bin/env bash

source `dirname "${BASH_SOURCE[0]}"`/../scripts/run.sh

run /usr/bin/time -f "[TIME]: %C %E" ./utils/common/precalc_caches.py --no-nanny

MYARGS=()
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/common/update_igroups.py -a checkcards')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_instances.py --only-dynamic')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_group_intersections.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_intlookups.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_unused_instances.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_tag_names.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_slave_groups_hosts.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_porto_limits.py -g ALL --check-net')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_porto_limits.py -g ALL_DYNAMIC --check-ssd --check-hdd --check-net')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_inherited_tags.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_hamster_shards.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_group_intlookups.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_shifted_groups.py -g ALL_DYNAMIC')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_hbf.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_tiers.py')
MYARGS+=('/usr/bin/time -f "[TIME]: %C %E" ./utils/check/check_indexerproxy_intersect.py')

par "${MYARGS[@]}"

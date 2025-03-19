#!/usr/bin/env bash

[[ -n "$DEBUG" ]] && [[ "$DEBUG" != "0" ]] && set -x

limit=100

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--cores) cores="$2"; shift 2;;
        -m|--memory) memory="$2"; shift 2;;
        -s|--ssd) ssd="$2"; shift 2;;
        -S|--sata) sata="$2"; shift 2;;
        -g|--generation) generation="$2"; shift 2;;
        -G|--geo) geo="$2"; shift 2;;
        -l|--limit)   limit="$2"; shift 2;;
        -h|--help)    help=1; shift; break;;
        --)           shift; break;;
        -*)           help=1; error=1; break;;
        *)            break;;
    esac
done

if [[ "$help" == 1 ]]; then
    cat <<EOF
Usage: `basename $0` [<option>]
  -c, --cores <cores>
  -m, --memory <memory_GB>
  -s, --ssd <ssd_GB>
  -S, --sata <sata_GB>
  -g, --generation <generation>
  -G, --geo <geo>
  -l, --limit <limit>
  -h, --help                                    Show this help message and exit.
EOF
    exit ${error:0}
fi


if [[ "$cores" == 'e' ]]; then
    cat <<EOF
Need to specify cores
EOF
    exit ${error:1}
else
    cores=$cores
fi

if [[ "$memory" == '' ]]; then
    cat <<EOF
Need to specify memory
EOF
    exit ${error:1}
else
    memory=$memory
fi

if [[ "$generation" == '' ]]; then
    generation=0
else
    generation="$generation"
fi

if [[ "$ssd" == '' ]]; then
    ssd=0
else
    ssd="$ssd"
fi

if [[ "$sata" == '' ]]; then
    sata=0
else
    sata="$sata"
fi

`dirname $0`/saltdb.sh <<EOF
WITH dom0_hosts AS (
    SELECT
        fqdn,
        free_raw_disks_space_pretty,
        generation,
        case generation when $generation then 1 else 0 end as generation_hard_rank,
        case total_cores >= $cores when true then 1 else 0 end as total_cores_hard_rank,
        case total_ssd >= ($ssd * power(2,30)) and 0 <= total_ssd when true then 1 else 0 end as total_ssd_hard_rank,
        case total_raw_disks_space >= ($sata * power(2,30)) and 0 <= ($sata * power(2,30)) when true then 1 else 0 end as total_sata_hard_rank,
        case total_memory >= ($memory * power(2,30)) and 0 <= total_memory when true then 1 else 0 end as total_memory_hard_rank,
        free_cores::decimal / $cores as free_cores_rank,
        free_ssd::decimal / ($ssd * power(2,30)) as free_ssd_rank,
        case total_raw_disks_space >= ($sata * power(2,30)) and ($sata * power(2,30)) != 0 when true then free_raw_disks_space::decimal / ($sata * power(2,30)) else case ($sata * power(2,30)) != 0 when true then 0 else 100500 end end as free_sata_rank,
        free_memory::decimal / ($memory * power(2,30)) as free_memory_rank,
        free_cores,
        free_memory_pretty,
        free_ssd_pretty,
        total_sata,
        geo,
        to_char(heartbeat, 'YYYY-MM-DD HH24:MI') heartbeat
    FROM mdb.dom0_info i
    WHERE
      (i.project = 'pgaas')
      AND ( i.generation = $generation)
      AND (i.geo = '$geo' OR '$geo' = '')
      AND (free_net > 0 AND free_io > 0)
      AND ($sata > 0 OR ($sata = 0 and 0 = 0 ))
)
SELECT fqdn, free_cores,
       free_memory_pretty as free_memory,
       free_ssd_pretty as free_ssd,
       free_raw_disks_space_pretty as free_sata,
       trunc(free_memory_rank::numeric, 2) as free_memory_rank,
       trunc(free_sata_rank::numeric, 2) as free_sata_rank,
       trunc(free_ssd_rank::numeric, 2) as free_ssd_rank,
       trunc(free_cores_rank::numeric, 2) as free_cores_rank,
       trunc(least(free_memory_rank, free_cores_rank, free_sata_rank, free_ssd_rank, 2)::decimal, 2) as free_rank,
       heartbeat,
       geo
FROM dom0_hosts
where (generation_hard_rank * total_cores_hard_rank * total_ssd_hard_rank * total_sata_hard_rank * total_memory_hard_rank) > 0
order by free_rank desc
LIMIT ${limit:-NULL};
EOF

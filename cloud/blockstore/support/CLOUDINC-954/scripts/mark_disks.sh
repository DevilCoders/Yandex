#!/bin/bash

resFile='disks_all_marked.txt'

cp disks_all.txt $resFile

cat tablets_all.txt | grep partition | awk '{print $3}' > disks_partition_damaged.txt
cat tablets_all.txt | grep volume | awk '{print $3}' > disks_volume_damaged.txt
grep -Fvf disks_partition_damaged.txt disks_volume_damaged.txt > disks_only_volume_damaged.txt

function mark_disks {
  grep -Ff $2 $1 | sed 's/$/ '$3'/' > suffix.txt
  grep -Fvf $2 $1 > other.txt
  cat other.txt >> suffix.txt
  sort suffix.txt > $1
}

mark_disks $resFile disks_not_damaged.txt 'not_damaged'
mark_disks $resFile disks_only_volume_damaged.txt 'only_volume_damaged'
mark_disks $resFile disks_repaired.txt 'repaired'
mark_disks $resFile disks_deleted.txt 'deleted'
mark_disks $resFile disks_dead.txt 'dead'
mark_disks $resFile disks_with_reset_partition.txt 'reset_partition'

grep -vE 'not_damaged|only_volume_damaged|repaired|deleted|dead|reset_partition' $resFile > none.txt
mark_disks $resFile none.txt 'none'


#!/usr/bin/env bash

./find_reused_devices.py --disk-device-events disk_device_events_20210301_20210821.txt --test-disk-ids test_disk_ids.txt --disk-id-to-cloud-id disk_id_to_cloud_id_full.txt --cloud-id-to-info ../cloud_info/clouds_res.csv --existing-disk-ids ./existing_disk_ids_20210822.txt --device-migration-events ./device_migrations.txt --disk-ids-from-images nrds_from_image.txt --compromised-by-existing 2> filtered_log5.txt > filtered_output5.txt &&
./find_reused_devices.py --disk-device-events disk_device_events_20210301_20210821.txt --test-disk-ids test_disk_ids.txt --disk-id-to-cloud-id disk_id_to_cloud_id_full.txt --cloud-id-to-info ../cloud_info/clouds_res.csv --existing-disk-ids ./existing_disk_ids_20210822.txt --device-migration-events ./device_migrations.txt --disk-ids-from-images nrds_from_image.txt 2> full_log5.txt > full_output5.txt &&
cat full_log5.txt | grep alien > final_alien2.txt &&
./fetch_solomon_stats.py --disks-with-alien-data final_alien2.txt | tee final_alien_with_stats2.txt &&
echo "DONE"

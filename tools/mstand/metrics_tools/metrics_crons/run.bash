#!/usr/bin/env bash
cron_ids='8792 8794 8796 8798 8800 8848 8850'
./mc-download-crons.py --cron-ids ${cron_ids} --token $1
./mc-update-crons.py --cron-ids ${cron_ids} --history-filters-config kpi_graphs_config.json
./mc-upload-crons.py --cron-ids ${cron_ids} --token $1

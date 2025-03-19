#!/usr/bin/env bash

salt-call mdb_clickhouse.create_zkflock_id zk_hosts='{{ zk_hosts | tojson }}' wait_timeout={{ wait_timeout }}

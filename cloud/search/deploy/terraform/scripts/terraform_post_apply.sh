#!/bin/sh
terraform output --json | jq '. | {"id":.search_robot_consumer_id.value, "key_algorithm":"RSA_2048","public_key": .search_robot_consumer_public_key.value, "private_key": .search_robot_consumer_private_key.value, "service_account_id": .search_robot_consumer_sa_id.value }' > sa_consumer.json

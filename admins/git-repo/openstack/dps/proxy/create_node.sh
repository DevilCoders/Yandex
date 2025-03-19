#!/bin/bash

#436e343f-bb90-4207-a69c-5ee85466233f - presice

nova --debug --os-region-name i boot --flavor m1.xlarge --image 436e343f-bb90-4207-a69c-5ee85466233f --key-name azalio --file /root/.ssh/authorized_keys=configs/authorized_keys --file /etc/apt/sources.list=configs/sources.list proxy01h.dps.yandex.net


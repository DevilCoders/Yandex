#!/bin/bash

# should be replace with logic, which chooses config
group_config=/root/rdiff-dev/config.file

. ${group_config}

echo $method

${method} ${group_config}

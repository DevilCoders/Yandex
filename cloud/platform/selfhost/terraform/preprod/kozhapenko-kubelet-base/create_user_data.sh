#!/usr/bin/env sh
set -x
cloud_config_parts_dir="module/files/user-data-parts"
user_data_output_path="user_data.out"

rm -f ${user_data_output_path}
find ${cloud_config_parts_dir} -name '*.yaml' -type f \
    -exec module/files/make-mime.py \
    -a {}:cloud-config >> ${user_data_output_path} \;

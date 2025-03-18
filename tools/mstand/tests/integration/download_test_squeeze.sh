#!/bin/sh

set -euo pipefail

mkdir -p mstand_test_data

cp run_dir/input_pool_filtered.json mstand_test_data/pool.json

dest_dir="mstand_test_data/squeeze/testids/web"
yt_base_dir="//home/mstand/test_data/squeeze/testids/web"

for table in $(yt find "${yt_base_dir}" --type table | sort); do
	local_path=$(echo "${table}" | sed s@${yt_base_dir}@${dest_dir}@g)

	if [ -f "${local_path}" ]; then
		echo "Already downloaded ${table}"
	else
		echo "Downloading ${table} to ${local_path}"

		tmpfile="$(mktemp)"
		yt read --format "<encode_utf8=false>json" "${table}" > "${tmpfile}"

		mkdir -p "$(dirname "${local_path}")"
		mv "${tmpfile}" "${local_path}"
	fi
done

echo "Archiving..."
tar cvf "mstand_test_data.tar.xz" mstand_test_data

echo "to upload to Sandbox:"
echo "$ ya upload mstand_test_data.tar.xz --ttl=365"

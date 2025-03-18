#!/bin/bash

# Updates test data

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
mapsmobi_dir=$(cd $script_dir/../../../maps/mobile/ && pwd)
base_idl_dir=$script_dir/../

test_data_dir=$base_idl_dir/tests/archive

rm -fr $test_data_dir
mkdir -p $test_data_dir $test_data_dir/idls $test_data_dir/frameworks

cp -r $mapsmobi_dir/../doc/proto/ $test_data_dir/protos

cp $mapsmobi_dir/libs/idl_frameworks/*.framework $test_data_dir/frameworks/
for bundle in "libs/runtime" "libs/recording" "libs/masstransit_recorder" "libs/push" "libs/auth" "libs/datasync" "libs/mapkit" "libs/bookmarks" "libs/places" "libs/transport" "libs/search" "libs/directions"; do
    for idl_dir in $(find "$mapsmobi_dir/$bundle" -name idl -type d -not -path "*/genfiles/*" -not -path "*/build/*"); do
        cp -r $idl_dir/* $test_data_dir/idls/
    done
done

idls=$(find $test_data_dir/idls -name *.idl -type f -print | sed s#$test_data_dir/idls/## | sort -r)

echo "All build"
$script_dir/../bin/tools-idl-app --in-proto-root $test_data_dir/protos \
                            -F$test_data_dir/frameworks \
                            -I$test_data_dir/idls \
                            --base-proto-package yandex.maps.proto \
                            --out-base-root $test_data_dir/all/base \
                            --out-android-root $test_data_dir/all/android \
                            --out-ios-root $test_data_dir/all/ios \
                            $idls || exit 1

echo "Public build"
$script_dir/../bin/tools-idl-app --in-proto-root $test_data_dir/protos \
                            -F$test_data_dir/frameworks \
                            -I$test_data_dir/idls \
                            --base-proto-package yandex.maps.proto \
                            --out-base-root $test_data_dir/public/base \
                            --out-android-root $test_data_dir/public/android \
                            --out-ios-root $test_data_dir/public/ios \
                            --public \
                            $idls || exit 1

tar cjf $test_data_dir.tbz2 -C $base_idl_dir ${test_data_dir#$base_idl_dir/}
rm -fr $test_data_dir

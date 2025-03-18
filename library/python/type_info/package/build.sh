set -euxo pipefail

version=`ya tool jq -r '.meta.version' pkg.json`

wheel_platforms=("manylinux1_x86_64" "macosx_11_0_arm64" "macosx_10_4_intel")
target_platforms=("CLANG14-LINUX-X86_64" "CLANG14-DARWIN-ARM64" "CLANG14-DARWIN-X86_64")

for i in "${!wheel_platforms[@]}"
do
    wheel_platform="${wheel_platforms[i]}"
    target_platform="${target_platforms[i]}"
    for python_version in 3.6 3.7 3.8 3.9 3.10 2.7
    do
        if [[
            ($python_version == "3.6" ||  $python_version == "3.7" || $python_version == "2.7") \
            && $wheel_platform == "macosx_11_0_arm64"
        ]]; then
            continue
        fi
        major="${python_version%.*}"
        minor="${python_version#*.}"
        flags=
        if [[ $major == "3" ]]
        then
            flags="$flags --wheel-python3"
        fi
        ya package \
            $flags \
            --wheel pkg.json \
            -DPYTHON_CONFIG=python$python_version \
            -DUSE_SYSTEM_PYTHON=$python_version \
            --wheel-platform $wheel_platform \
            --target-platform $target_platform
        abi=cp${major}${minor}
        correct_name="yandex_type_info-${version}-cp${major}${minor}-${abi}-${wheel_platform}.whl"
        mv \
            yandex_type_info-${version}-py${major}-none-${wheel_platform}.whl \
            $correct_name
        twine upload -r yandex "$correct_name"
    done
done
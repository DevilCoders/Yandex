#!/bin/bash

prefix=libvhost-server
git_hash=$(git rev-parse --short HEAD)
test_dir_template="${TEAMCITY_BUILDCONF_NAME}-${git_hash}-XXXXXX"

gen_src_tgz()
{
    git archive --prefix=$prefix/ --format=tar.gz HEAD
}

prep_work_dir()
{
    ssh "$REMOTE_TEST_SERVER" "
        set -e
        root_dir=$REMOTE_TEST_ROOT
        test_dir=\$(mktemp -d \"\$root_dir/$test_dir_template\")
        tar -xzf - -C \"\$test_dir\"
        echo \"\$test_dir\"
    "
}

mk_work_dir()
{
    gen_src_tgz | prep_work_dir
}

ensure_work_dir()
{
    # $work_dir is global; mk_work_dir is only called once
    : ${work_dir:=$(mk_work_dir)}
}

run_test_remote()
{
    ensure_work_dir
    ssh "$REMOTE_TEST_SERVER" "
        set -xe
        root_dir=$REMOTE_TEST_ROOT
        mkdir -p \"\$root_dir\"
        export TEST_CACHE_DIR=\"\$root_dir/cache\"
        test_dir=\"$work_dir\"
        cd \"\$test_dir/$prefix\"
        $@
    "
}

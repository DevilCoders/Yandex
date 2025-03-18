#!/usr/bin/env bash

main() {
    success=1

    what="$1"
    dest_dir="$2"
    shift
    shift

    mkdir -p "$dest_dir" || success=0
    if [ ${success} -ne 1 ]; then
        echo "Cannot create \"$dest_dir\""
        return 1
    fi

    get_gencfg_repo_url="http://api.gencfg.yandex-team.ru/static/get_gencfg_repo.py"

    if [ `uname` == 'Linux' ]; then
        get_gencfg_repo_path=`mktemp`
    elif [ `uname` == 'FreeBSD' ]; then
        get_gencfg_repo_path=`mktemp /var/tmp/tmp.XXXXXXXXXX`
    fi

    curl -f "$get_gencfg_repo_url" >"$get_gencfg_repo_path" 2>/dev/null || success=0
    if [ ${success} -ne 1 ]; then
        echo "Cannot download \"$get_gencfg_repo_url\""
        return 1
    fi

    chmod 777 "$get_gencfg_repo_path" || success=0
    if [ ${success} -ne 1 ]; then
        echo "Cannot chmod \"$get_gencfg_repo_path\""
        return 1
    fi

    "$get_gencfg_repo_path" "-t" "$what" "-d" "$dest_dir" $@ || success=0
    if [ ${success} -ne 1 ]; then
        echo "Download script failed!"
        return 1
    fi
    if [ "$what" == "full" ]; then
        old_dir=`pwd`
        cd "$dest_dir"
        ./checkout.sh || success=0
        if [ ${success} -ne 1 ]; then
            echo "\"checkout.sh\" failed!"
            cd "$old_dir"
            return 1
        fi
        ./pull.sh || success=0
        if [ ${success} -ne 1 ]; then
            echo "\"pull.sh\" failed!"
            cd "$old_dir"
            return 1
        fi
        cd "$old_dir"
    else
        old_dir=`pwd`
        cd "$dest_dir"
        "/usr/bin/env" "git" "pull" >/dev/null 2>&1 || success=0
        if [ ${success} -ne 1 ]; then
            echo "\"git pull\" failed!"
            cd "$old_dir"
            return 1
        fi
        cd "$old_dir"
    fi

    return 0
}

if main $@; then
    # works well when running by "source" command
    return 0 2> /dev/null || exit 0
else
    echo "Failed"
    # works well when running by "source" command
    return 1 2> /dev/null || exit 1
fi

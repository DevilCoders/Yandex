#!/usr/bin/env bash

sandbox=$1

source `dirname "${BASH_SOURCE[0]}"`/scripts/run.sh

MYDIR=`dirname "${BASH_SOURCE[0]}"`

if [ "$#" -ge 3 ]; then
    echo "Usage: $0 [(skip_resource|skip_venv)] [(tests|prod)]"
    exit 1
fi

if [ "$#" -eq 0 ]; then
    MODE="tests"
elif [ "$#" -eq 1 ] && ([ "$sandbox" == "skip_resource" ] || [ "$sandbox" == "skip_venv" ]); then
    echo "1 && (skip_resource || skip_venv)"
    MODE="tests"
elif [ "$#" -eq 2 ] && ([ "$sandbox" == "skip_resource" ] || [ "$sandbox" == "skip_venv" ]); then
    echo "2 && (skip_resource || skip_venv)"
    MODE=$2
else
    MODE=$1
    sandbox=""
fi


echo "Checking out db"
if [ ! -d "db" ]; then
    svn co -q svn+ssh://arcadia.yandex.ru/arc/trunk/data/gencfg_db db
fi

echo "Upgrading to current svn version"
svn upgrade
(cd db; svn upgrade)

if [ "$sandbox" == "skip_resource" ] || [ "$sandbox" == "skip_venv" ]; then
    echo "Skipping download wheels"
else
    echo "Downloading wheels"
    download_sandbox_resource wheels
    download_sandbox_resource binutils
fi

OLDPYTHONPATH=`echo ${PYTHONPATH}`

if [ "$sandbox" == "skip_venv" ]; then
    echo "Skipping install virtual environment..."
else
    echo "Building shared virtualenv..."

    unset "PYTHONPATH"

    run rm -rf ${MYDIR}/venv
    run mkdir -p ${MYDIR}/venv/venv
    echo "This directory was created intentionally, to avoid skynet virtualenv troubles" > ${MYDIR}/venv/readme.txt
    VENVPATH=`realpath ${MYDIR}/venv/venv`


    if [ "${MODE}" == "prod" ]; then
        # use TMPLINK because virtualenv command fails on paths with colon (when run on bsconfig)
        TMPLINK=`mktemp -u venv.XXXXXXXXXX --tmpdir=/var/tmp`
        run ln -s ${VENVPATH} ${TMPLINK}
        run /skynet/python/bin/virtualenv ${TMPLINK}
    elif [ "${MODE}" == "tests" ]; then
        # but if we use TMPLINK then testing (py.test) fail...
        run /skynet/python/bin/virtualenv ${VENVPATH}
    else
        echo "========================================================="
        echo "Mode can not be <${MODE}>, exiting ..."
        echo "========================================================="
        exit 1
    fi

    echo "Installing common packages..."
    run source venv/venv/bin/activate

    run venv/venv/bin/python venv/venv/bin/pip install -U ${MYDIR}/wheels/pip-7.1.0.tar.gz
    run venv/venv/bin/python venv/venv/bin/pip install --no-index --find-links=${MYDIR}/wheels -r ${MYDIR}/pip.reqs.txt
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ pip==20.1.1 --upgrade
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ yandex-yp-skynet==0.4.3.post0
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ yandex-yt-orm==1.0.2
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ py
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ ipaddress
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ yandex-yt
    run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ protobuf==3.17.3
    #run venv/venv/bin/python venv/venv/bin/pip install -i https://pypi.yandex-team.ru/simple/ startrek_client --upgrade
    #run venv/venv/bin/python venv/venv/bin/pip install --upgrade pip

    cp ${MYDIR}/version ${MYDIR}/venv

    run deactivate

    if [ "${MODE}" == "tests" ]; then
        echo "Installing packages for balancer_gencfg"
        run custom_generators/balancer_gencfg/install.sh
    fi
fi


export PYTHONPATH=${OLDPYTHONPATH}

if [ "$sandbox" == "skip_venv" ]; then
    rm -f ${TMPLINK}
fi

echo "Making workaround symlinks"
ln -sfT gaux aux

echo "Update sandbox installed resources"
cp ${MYDIR}/sandbox.resources ${MYDIR}/sandbox.resources.installed

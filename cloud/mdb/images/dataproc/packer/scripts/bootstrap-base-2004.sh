#!/bin/bash
# Script for installing base components for Yandex Cloud Data-Proc image 2.x.
# This scripts install infrastructure components like saltstack, repositories, drivers and conda environment

set -x -euo pipefail
DAEMON_NAME=packer-dataproc-base

# Directory for temporary files
TMPDIR=$(mktemp -d)

SALT_VERSION="3002.6"
MINICONDA_VERSION="py38_4.9.2"
CONDA_INSTALL_PATH="/opt/conda"

log_fatal (){
    logger -ip daemon.error -t ${DAEMON_NAME} "$@"
    exit 1
}

log_info (){
    logger -ip daemon.info -t ${DAEMON_NAME} "$@"
}

setup_apt_mirrors() {
    # Setup salt repository
    REPO="https://repo.saltproject.io/py3/ubuntu/20.04/amd64/3002/"
    cat <<EOF > "/etc/apt/sources.list.d/saltstack.list"
deb [arch=amd64] ${REPO} focal main
EOF
    saltkey="${TMPDIR}/salt.gpg"
    curl -fS --connect-time 3 --max-time 5 --retry 10 "${REPO}SALTSTACK-GPG-KEY.pub" -o $saltkey || log_fatal "Can't download saltstack gpg key"
    apt-key add $saltkey || log_fatal "Can't add salt gpg key"

    # Setup postgres repository
    REPO="http://mirror.yandex.ru/mirrors/postgresql/"
    cat <<EOF > "/etc/apt/sources.list.d/postgres.list"
deb [arch=amd64] ${REPO} focal-pgdg main
EOF
    pgdgkey="${TMPDIR}/pgdg.gpg"
    curl -fS --connect-time 3 --max-time 5 --retry 10 "${REPO}ACCC4CF8.asc" -o $pgdgkey || log_fatal "Can't download postgres gpg key"
    apt-key add $pgdgkey || log_fatal "Can't add postgres gpg key"

    DEBIAN_FRONTEND=noninteractive LANG=C apt-get clean || log_fatal "Can't clean apt"
    DEBIAN_FRONTEND=noninteractive LANG=C apt-get update || log_fatal "Can't update apt cache"
}

system_upgrade(){
    DEBIAN_FRONTEND=noninteractive LANG=C apt-get purge -y unattended-upgrades || log_fatal "Can't purge unattended-upgrades"
    DEBIAN_FRONTEND=noninteractive LANG=C apt-get install -y locales || log_fatal "Can't install locales"
    locale-gen en_US.UTF-8 || log_fatal "Failed to set locales"
    dpkg-reconfigure --frontend=noninteractive locales || log_fatal "Can't rebuild locales"
    DEBIAN_FRONTEND=noninteractive apt-get -o Dpkg::Options::='--force-confnew' upgrade -y || log_fatal "Can't upgrade installed packages"
}

setup_utils() {
    DEBIAN_FRONTEND=noninteractive apt-get install -y rsyslog \
        openssh-server \
        curl \
        strace \
        lsof \
        tcpdump \
        sysstat \
        less \
        inetutils-ping \
        bzip2 \
        ufw \
        rsync \
        htop || log_fatal "Can't install utils"
}

setup_salt() {
    DEBIAN_FRONTEND=noninteractive apt-get install -y salt-minion salt-common virt-what python3-retrying python3-pip || log_fatal "Can't install salt"
    systemctl disable salt-minion || log_fatal "Failed to disable salt-minion on autostart"
}

setup_conda() {
    if [[ -d "${CONDA_INSTALL_PATH}" ]]; then
        log_info "${CONDA_INSTALL_PATH} exists. Skipping"
        return
    fi
    REPO="https://repo.anaconda.com/miniconda/"
    MINICONDA_SCRIPT_PATH="${TMPDIR}/Miniconda3-Linux-x86_64.sh"
    curl -fS --connect-time 3 --max-time 120 --retry 10 "${REPO}Miniconda3-${MINICONDA_VERSION}-Linux-x86_64.sh" -o $MINICONDA_SCRIPT_PATH || log_fatal "Can't download conda installer"

    log_info "Installing conda"
    bash $MINICONDA_SCRIPT_PATH -b -p $CONDA_INSTALL_PATH -f || log_fatal "Failed to install miniconda"
    CONDA_BIN_PATH="$CONDA_INSTALL_PATH/bin"
    echo "Updating global profiles to export miniconda bin location to PATH..."

    # Load conda environment
    CONDA_PROFILE="/etc/profile.d/conda.sh"
    ln -s /opt/conda/etc/profile.d/conda.sh $CONDA_PROFILE
    DATAPROC_PROFILE="/etc/profile.d/dataproc.sh"
    if grep -ir "# Data Proc profile" $DATAPROC_PROFILE
    then
        log_info "$DATAPROC_PROFILE exists, skipping"
    else
        log_info "Adding path definition to profiles"
        echo "# Data Proc profile" | tee -a $DATAPROC_PROFILE
        echo "# Load variables from hadoop and spark configs" | tee -a $DATAPROC_PROFILE
        echo "test -f /etc/default/hadoop && source /etc/default/hadoop" | tee -a $DATAPROC_PROFILE
        echo "test -f /etc/default/hadoop-yarn-resourcemanager && source /etc/default/hadoop-yarn-resourcemanager" | tee -a $DATAPROC_PROFILE
        echo "test -f /etc/default/hadoop-yarn-nodemanager && source /etc/default/hadoop-yarn-nodemanager" | tee -a $DATAPROC_PROFILE
        echo "test -f /etc/spark/conf/spark-env.sh && source /etc/spark/conf/spark-env.sh" | tee -a $DATAPROC_PROFILE
        echo "export CONDA_BIN_PATH=$CONDA_BIN_PATH" | tee -a $DATAPROC_PROFILE
        echo "export PATH=$CONDA_BIN_PATH:$PATH" | tee -a $DATAPROC_PROFILE
        echo "# Enable conda's python for pyspark" | tee -a $DATAPROC_PROFILE
        echo "export PYSPARK_PYTHON=$CONDA_BIN_PATH/python" | tee -a $DATAPROC_PROFILE
    fi

    $CONDA_BIN_PATH/conda update --all --quiet --yes || log_fatal "Failed to update conda packages"
    $CONDA_BIN_PATH/conda config --append channels conda-forge || log_fatal "Failed to enable conda-forge channel"
    # Increase timeout and attempts for downloading packages
    $CONDA_BIN_PATH/conda config --set remote_connect_timeout_secs 15.0
    $CONDA_BIN_PATH/conda config --set remote_max_retries 5
}

setup_nvidia_drivers () {
    if [[ "${ENABLE_GPU}" == "false" ]]
    then
        echo "GPU disabled, skipping nvidia drivers."
        return
    fi
    # Setup nvidia repository
    REPO="http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1604/x86_64"
    cat <<EOF > "/etc/apt/sources.list.d/nvidia.list"
deb ${REPO} /
EOF
    key="${TMPDIR}/salt.gpg"
    curl -fS --connect-time 3 --max-time 5 --retry 10 "${REPO}/7fa2af80.pub" -o $key || log_fatal "Can't download nvidia repository key"
    apt-key add $saltkey || log_fatal "Can't add nvidia gpg key"
    DEBIAN_FRONTEND=noninteractive apt-get update -qq || log_fatal "Can't update repository cache"
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends cuda-drivers || log_fatal "Can't update repository cache"
}

setup_grpcurl() {
    GRPCURL_ARCHIVE="https://github.com/fullstorydev/grpcurl/releases/download/v1.6.0/grpcurl_1.6.0_linux_x86_64.tar.gz"
    curl -fS -L --connect-time 3 --max-time 30 --retry 10 "${GRPCURL_ARCHIVE}" -o ${TMPDIR}/grpcurl.tar.gz || log_fatal "Can't download grpcurl"
    pushd ${TMPDIR}
    tar -zxvf grpcurl.tar.gz || log_fatal "Can't extract grpcurl"
    cp ${TMPDIR}/grpcurl /usr/local/bin/grpcurl || log_fatal "Can't install grpcurl"
    popd
    ln -s /usr/local/bin/grpcurl /usr/bin/grpcurl
}

clean_apt() {
    DEBIAN_FRONTEND=noninteractive apt-get clean || log_fatal "Can't clean apt"
}

cleanup () {
    rm -rf ${TMPDIR}
    rm -rf /var/log/apt/*
    rm -rf /var/log/{auth,dpkg,syslog,wtmp,kern,alternatives,auth,btmp,dmesg}.log
    rm -rf /etc/network/interfaces.d/*
    rm -rf /var/lib/dhcp/*

}

main() {
    setup_apt_mirrors
    system_upgrade
    setup_utils
    setup_grpcurl
    setup_salt
    setup_conda
    clean_apt
    cleanup
}

main

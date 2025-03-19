FROM registry.yandex.net/ubuntu:trusty

# For yandex-search-common-apt
RUN echo $'deb http://common.dist.yandex.ru/common stable/all/\ndeb http://common.dist.yandex.ru/common stable/amd64/\n' >> etc/apt/sources.list.d/yandex-common-temporary.list 

RUN export DEBIAN_FRONTEND="noninteractive"
RUN apt-get update 

ENV APT_GET="apt-get --yes"
RUN $APT_GET --force-yes install yandex-archive-keyring yandex-search-common-apt && \
    rm etc/apt/sources.list.d/yandex-common-temporary.list && \
    apt-get update

RUN $APT_GET --force-yes dist-upgrade

# Time zone: MSK
RUN $APT_GET install tzdata && \
    echo 'Europe/Moscow' | tee /etc/timezone && \
    dpkg-reconfigure -f noninteractive tzdata

# Locale: en_US.UTF-8
RUN $APT_GET install language-pack-en && \
    update-locale LANG=en_US.UTF-8

# Packages required by our ISS hooks and guaranteed to exist in the docs
RUN $APT_GET install \
    libticket-parser \
    libnss-myhostname \
    coreutils bc sed gawk tar gzip xz-utils bzip2 diffutils patch findutils grep binutils dnsutils lsof \
    bsdutils bsdmainutils util-linux \
    bash python2.7 perl \
    python-async python-argparse python-importlib python-chardet python-configobj python-ConfigParser python-six python-psutil python-doc python-xattr python-sss python-ctypes python-jinja2 python-markupsafe python-email python-requests python-ipaddr python-flask python-werkzeug python-wsgiref \
    curl rsync wget netcat-openbsd net-tools traceroute yandex-internal-root-ca ca-certificates \
    yandex-search-user-loadbase \
    strace ltrace gdb libc6-dbg \
    less vim-tiny atop htop procps time
# yandex-porto

# vmtouch (built from source in nirvana container)
# Sandbox resource: https://sandbox.yandex-team.ru/resource/1181172737/view
RUN mkdir -p /usr/local/bin && \
    curl -o /usr/local/bin/vmtouch https://proxy.sandbox.yandex-team.ru/1181172737 && \
    chmod +x /usr/local/bin/vmtouch

# Add libsunec.so from JRE for job_launcher_native
# Sandbox resource: https://proxy.sandbox.yandex-team.ru/1152317904/view
RUN mkdir -p /opt/nirvana-jre/lib/amd64 && \
    curl -o /opt/nirvana-jre/lib/amd64/libsunec.so https://proxy.sandbox.yandex-team.ru/1152317904

# add YT-specific users and group
RUN for i in {0..99}; do echo yt_slot_$i:x:199$(printf "%02d" $i):1001::/home/yt_slot_$i:/bin/false >> /etc/passwd; done && \
    echo yt:x:1001:1001::/yt:/bin/false >> /etc/passwd && \
    echo yt:x:1001: >> /etc/group

# create a directory for core dumps, owned by the user executing the jobs
RUN mkdir /coredumps && chown -R loadbase:loadbase /coredumps
# set the coredump pattern
RUN sysctl -w kernel.core_pattern='/coredumps/%e.%p.%s'

# default ubuntu has line "hosts: files myhostname dns" in /etc/nsswitch.conf
# and "myhostname" record forces getaddrinfo() to return fastbone ip addresses that causes network errors
RUN sed -i "s/^hosts:.*/hosts:\tfiles dns/g" /etc/nsswitch.conf

RUN apt-get clean && \
    apt-get autoclean && \
    rm -rf /tmp/* /var/lib/apt/lists/* /var/cache/apt/*.bin && \
    find /var/log -iname '*.gz' -delete && \
    find /var/log -iname '*.1' -delete && \
    find /var/log -type f -print0

ENV NV_BUFFER_SIZE "_128_MB"
ENV NV_JL_XMX "2080"
ENV NV_JL_XX_MAX_DIRECT "128"
ENV NV_JL_XX_MAX_METASPACE_SIZE "80"
ENV NV_JL_XX_RESERVED_CODE_CACHE "48"
ENV NV_LOG_DIR_PREFIX "/nv/log/"
ENV NV_MTN_PROJECT_ID "4456"
ENV NV_NATIVE_JOB_LAUNCHER "true"
ENV NV_SYS_DIR_PREFIX "/nv/sys/"
ENV NV_TMPFS_SANDBOX "false"

RUN mkdir /nv && mkdir /nv/sys && mkdir /nv/log && mkdir /nv/sys/static
RUN mkdir /nv_tmpfs &&  mkdir /nv_tmpfs/sys/ && mkdir /nv_tmpfs/sys/static/ 
RUN touch /nv_tmpfs/sys/hosts.txt

COPY target/job_context.ftl /nv_tmpfs/sys/static/job_context.ftl
COPY target/job_context_base.ftl /nv_tmpfs/sys/static/job_context_base.ftl

# If non-native job launcher version
# COPY target/nirvana-job-launcher-1.0-SNAPSHOT.jar /nv/sys/static/job_launcher.jar

COPY target/job_launcher.sh /nv/sys/static/job_launcher.sh
COPY target/job_launcher.vmoptions_short /nv/sys/static/job_launcher.vmoptions_short
COPY target/job_launcher_locked_files /nv/sys/job_launcher_locked_files
COPY target/jdkrun /nv/sys/static/jdkrun

COPY target/nirvana-job-launcher-micronaut-app-1.0-SNAPSHOT-native-image /nv/sys/static/job_launcher_native
RUN chmod +x /nv/sys/static/job_launcher_native

COPY target/job.json /nv/sys/job.json

CMD source ${NV_SYS_DIR_PREFIX}static/job_launcher.sh && jl_main

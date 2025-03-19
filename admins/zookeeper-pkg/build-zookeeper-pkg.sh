#!/bin/sh -e

die() {
    echo "$@" >&2
    exit 1
}

ec() {
    echo "$@" >&2
    "$@"
}

exec </dev/null

if [ -n "$TEAMCITY_VERSION" ]; then
    echo
    echo "=== Dumping environment"
    echo
    id
    pwd
    hostname
    echo
    env
    echo
    echo "=== done dumping environment"
    echo
fi

project="$1"
build_msg="${2:-fix}"

if [ -n "$3" ]; then
    DEBEMAIL="$3"
else
    if [ -n "${DEBEMAIL}" ]; then
        :;
    else
        DEBEMAIL="${USER}@yandex-team.ru"
    fi
fi

if [ -z "$project" ]; then
    die "
    usage: $0 <project> [ description [gpg-signer] ]

    example: $0 myproject
    example: $0 myproject 'fix'
    example: $0 myproject 'upgrade zk to version 1.2.3' myname@yandex-team.ru
    "
fi

PACKAGE="yandex-zookeeper-$project"
REST_NAME="yandex-zookeeper-rest-$project"
ZK_NAME="yandex-zookeeper-$project"
DIR="zookeeper-$project"

#
# zoo-$project.cfg is expected to have the following special properties:
#   yandex.user -- system user that will be used to run zk daemon
#   yandex.servers.$ENVIRONMENT -- either %conductor-group or csv:host1,host2,host3
#
#   zk_ver  - version of package to download
#   our_ver - version of package to build (use only: "alnum/+/./~", not "-")
#

echo "=== Building package with options:"

ec export DEBEMAIL="${DEBEMAIL}"
ec export CONFIG_TEMPLATE=$(pwd -P)/zoo-$project.cfg
ec export CONFIG_REST_TEMPLATE=$(pwd -P)/zoo-$project-rest.cfg
test -f $CONFIG_TEMPLATE || die "Missing config template $CONFIG_TEMPLATE"

# XXX should our package version include zk version?
# XXX zk 3.5.x require minimum jdk version 1.8
default_zk_ver="3.7.0"
default_our_ver="1.5.34"

project_zk_ver=$(awk  -F '=' '/^zk_ver/{print $2}'  ./zoo-$project.cfg | tr -d \'\")
project_our_ver=$(awk -F '=' '/^our_ver/{print $2}' ./zoo-$project.cfg | tr -d \'\")

ec export zk_ver=${project_zk_ver:-$default_zk_ver}
ec export our_ver=${project_our_ver:-$default_our_ver}

echo "=== (end options)\n\n"

echo "=== Get src"

URL=''
if echo $zk_ver | grep -q -E '^3[.](3|4|5.[0-4])'; then
    # use old names for version <= 3.5.4
    ARTIFACTS="zookeeper-${zk_ver}"
else
    ARTIFACTS="apache-zookeeper-${zk_ver}-bin"
fi

check_repos() {
    for test_url in \
        "https://downloads.apache.org/zookeeper/zookeeper-${zk_ver}"            \
        "https://archive.apache.org/dist/zookeeper/zookeeper-${zk_ver}"         \
        # "https://www.apache.org/dyn/closer.lua/zookeeper/zookeeper-${zk_ver}"   \
        # "http://mirrors.ibiblio.org/apache/zookeeper/zookeeper-${zk_ver}"       \
        # "http://www.motorlogy.com/apache/zookeeper/zookeeper-${zk_ver}"
    do
        if wget --timeout=2 --method=HEAD --quiet -- "${test_url}" >&2; then
            echo "${test_url}"
            return 0
        fi
    done
    echo "ERR: (check_repos) can not get repo and download src package." >&2
    return 1
}

URL=$(check_repos)
ec wget -c "${URL}/${ARTIFACTS}.tar.gz"


echo "=== Clear old pkg-dir"
ec rm -rf pkg-dir
ec mkdir pkg-dir
ec cd pkg-dir

echo "=== Prepare configs"
get_property() {
    FILE="$1"
    PROPERTY_NAME="$2"
    grep -m 1 "^${PROPERTY_NAME}=" "$FILE" | sed "s/^[^=]*=//"
}

if [ -f $CONFIG_REST_TEMPLATE ]; then
    REST_PORT=$(get_property $CONFIG_REST_TEMPLATE rest.port)
fi

PORT=$(get_property $CONFIG_TEMPLATE clientPort)
USER=$(get_property $CONFIG_TEMPLATE yandex.user)
test -n "$USER" || die "Property yandex.user undefined!"

TARGET_OS=$(get_property $CONFIG_TEMPLATE targetOs)
if [ "$TARGET_OS" = "" ]; then
    TARGET_OS=debian
fi

UBICINIT=$(get_property $CONFIG_TEMPLATE ubicinit)
if [ "$UBICINIT" = "" ]; then
    UBICINIT="YES"
fi

if [ "$(get_property $CONFIG_TEMPLATE java.network.ipv6)" = "true" ]; then
    JAVA_NETWORK_OPTION="-Djava.net.preferIPv6Addresses=true"
else
    JAVA_NETWORK_OPTION="-Djava.net.preferIPv4Stack=true"
fi

JAVA_ZOOKEEPER_OPTS="$(get_property $CONFIG_TEMPLATE zookeeper.opts)"

if [ "$(get_property $CONFIG_TEMPLATE authProvider.1)" = "org.apache.zookeeper.server.auth.SASLAuthenticationProvider" ]; then
    JAVA_SECURITY_OPTION="-Djava.security.auth.login.config=/etc/yandex/\$serviceName/jaas.conf"
else
    JAVA_SECURITY_OPTION=""
fi

JVM_MEM_XMS="$(get_property $CONFIG_TEMPLATE yandex.jvm.xms)"
if ! [ "$JVM_MEM_XMS" = "" ]; then
    JAVA_MEM_OPTION="-Xms$JVM_MEM_XMS"
fi
JVM_MEM_XMX="$(get_property $CONFIG_TEMPLATE yandex.jvm.xmx)"
if ! [ "$JVM_MEM_XMX" = "" ]; then
    JAVA_MEM_OPTION="$JAVA_MEM_OPTION -Xmx$JVM_MEM_XMX"
fi

deps=$(get_property $CONFIG_TEMPLATE dependencies)
DEPENDENCIES=${deps:-'yandex-jdk7 | yandex-jdk8, yandex-environment, syslog-ng-include, syslog-ng-include-tpl3, netcat-openbsd'}
if [ x"$UBICINIT" = x"YES" ]; then
    DEPENDENCIES="yandex-media-ubic, $DEPENDENCIES"
fi

echo "Using dependencies $DEPENDENCIES"

parse_group_or_hosts() {
    GROUP_OR_HOSTS="$1"
    if echo "$GROUP_OR_HOSTS" | egrep "^%" >/dev/null; then
        GROUP=$(echo "$GROUP_OR_HOSTS" | sed 's/^%//')
        ec wget --no-verbose -O - "http://c.yandex-team.ru/api/groups2hosts/${GROUP}?fields=fqdn"
    elif echo "$GROUP_OR_HOSTS" | egrep "^csv:" >/dev/null; then
        echo $GROUP_OR_HOSTS | sed 's/^csv://' | tr "," "\n"
    else
        die "Unknown group or hosts definition: [$GROUP_OR_HOSTS]"
    fi
}

for ENV in development stress testing intra qa production; do
    echo
    echo "Environment: $ENV"

    GROUP_OR_HOSTS=$(get_property $CONFIG_TEMPLATE yandex.servers.$ENV)
    if [ -z "$GROUP_OR_HOSTS" ]; then
        echo "Servers: none -- skipping."
        continue
    fi

    echo "Servers: $GROUP_OR_HOSTS"
    HOSTS=$(parse_group_or_hosts $GROUP_OR_HOSTS)

    CONFIG_TARGET=./zoo.cfg.$ENV

    cp $CONFIG_TEMPLATE $CONFIG_TARGET
    sed -i'' -r 's/^(zk_ver=|our_ver=)/#\1/' $CONFIG_TARGET

    echo "# Parsed/retrieved hosts at package build time:" | tee -a $CONFIG_TARGET
    echo "$HOSTS" | sort |
        awk "BEGIN {n=0} {print \"server.\" ++n \"=\" \$1 \":\" $PORT+1 \":\" $PORT+2}" |
        tee -a $CONFIG_TARGET

    echo "Generated config file $CONFIG_TARGET"

    if [ -f $CONFIG_REST_TEMPLATE ]; then
        CONFIG_REST_TARGET=./rest.cfg.$ENV
        cp $CONFIG_REST_TEMPLATE $CONFIG_REST_TARGET

        echo "# Parsed/retrieved hosts at package build time:" | tee -a $CONFIG_REST_TARGET
        echo "$HOSTS" | sort |
            awk "BEGIN {n=0;ORS=\",\";} {print \$1 \":\" 2181}" |
            sed -r 's/(.*),/rest.endpoint.1=\/;\1\n/' |
            tee -a $CONFIG_REST_TARGET

        echo "Generated REST config file $CONFIG_REST_TARGET"
    fi
done

echo
ec pwd
ec cp -rlv ../debian ./debian
ec cp -rlv ../src ./src

echo
ec tar -xf ../${ARTIFACTS}.tar.gz
(
    ec cd ${ARTIFACTS}
    (cd src/contrib/rest && ant ivy-retrieve) || true
    ec cp -R contrib/rest/* . || true
    ec cp build/contrib/rest/lib/* lib || true
    ec rm -rf src
    ec rm -rf docs
    ec rm -rf contrib
    ec rm -rf recipes
    ec rm -rf bin/*.cmd
)

detemplatize() {
    for f in "$@"; do
        ec sed -e '/@@@replace-whole-line-with-post-inst@@@/{r ../generic-postinst.sh' -e 'd}' -i $f
        ec sed -e "
s,@package@,$PACKAGE,g
s,@project@,$project,g
s,@dir@,$DIR,g
s,@user@,$USER,g
s,@port@,$PORT,g
s,@zkName@,$ZK_NAME,g
s,@restName@,$REST_NAME,g
s,@restPort@,$REST_PORT,g
s;@dependencies@;$DEPENDENCIES;g
s,@javaNetworkOption@,$JAVA_NETWORK_OPTION,g
s,@javaSecurityOption@,$JAVA_SECURITY_OPTION,g
s,@javaMemOption@,$JAVA_MEM_OPTION,g
s,@javaZookeeperOpts@,$JAVA_ZOOKEEPER_OPTS,g
s,@version@,$our_ver,g
" -i $f
    done
}

detemplatize rest.cfg.* || echo "Failed to detemplatize rest config"
detemplatize zoo.cfg.* src/*.sh src/*.properties src/*.pl src/cron src/zabbix.conf \
    src/checks/* src/monrun/*

if [ -n "$TEAMCITY_VERSION" ]; then
    # to make debsign work
    # TODO support rpm building
    echo "##teamcity[buildNumber '${our_ver} (zk=${zk_ver})']"
    export DEBEMAIL="teamcity@yandex-team.ru"
    export DEBFULLNAME="teamcity"
fi

# OS-specific code follows

echo -e '\n=== BUILDING'
pwd
ls -la

if [ "$TARGET_OS" = "debian" ]; then
    dpkg -l dh-make devscripts > /dev/null || echo -e "You should to install:\n  sudo apt-get install devscripts dh-make"

    if [ "$UBICINIT" = "NO" ]; then
        mv debian/init.debian.zk debian/$PACKAGE.$ZK_NAME.init
        mv debian/init.rest debian/$PACKAGE.$REST_NAME.init
    else
        mv debian/init.zk debian/$PACKAGE.$ZK_NAME.init
        mv debian/init.rest debian/$PACKAGE.$REST_NAME.init
    fi
    detemplatize debian/*

    ec dch --create --newversion "${our_ver}" --package $PACKAGE --distribution "unstable" --force-distribution "${build_msg}"
    ec debuild -e${3:-$DEBEMAIL} -b

elif [ "$TARGET_OS" = "redhat" ]; then
    cp ../zookeeper.spec.template zookeeper.spec
    cp ../redhat.init.template redhat.init
    detemplatize zookeeper.spec redhat.init

    rpmbuild -bb -v zookeeper.spec

else
    die Unknown targetOs=$TARGET_OS
fi

echo "=== Built package:"
ec cd -
ls -lah ${PACKAGE}_${our_ver}*

echo

# vim: set ts=4 sw=4 et:

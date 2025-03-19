#!/bin/sh -e

if [ ! -e /etc/zookeeper/conf/environment ]; then
    echo "File /etc/zookeeper/conf/environment not found, failing start"
    exit 199
fi
. /etc/zookeeper/conf/environment


exec $JAVA \
    -cp $CLASSPATH \
    $JAVA_OPTS \
    -Dzookeeper.log.dir=${ZOO_LOG_DIR} \
    -Dzookeeper.root.logger=${ZOO_LOG4J_PROP} \
{% if salt.pillar.get('data:zk:scram_auth_enabled', False) %}
    -Djava.security.auth.login.config=/etc/zookeeper/conf.yandex/zookeeper_jaas.conf \
{% endif %}
    $ZOOMAIN $ZOOCFG

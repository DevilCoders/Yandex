
        ENV=`cat /etc/yandex/environment.type`
        ENV_CONFIG=/etc/yandex/@dir@/zoo.cfg.$ENV

        if ! test -f $ENV_CONFIG ; then
            echo "FAIL: no zk config $ENV_CONFIG for current environment!"
            exit 1
        fi

        if ! grep "^server.[0-9]*=`hostname -f`:[0-9]*:[0-9]*$" $ENV_CONFIG ; then
            echo "FAIL: current host \"`hostname -f`\" is missing in $ENV_CONFIG!"
            exit 1
        fi

        # TODO check conductor group, too

        echo "Using zoo.cfg.$ENV ..."
        if [ -f /etc/yandex/@dir@/zoo.cfg ]; then
            rm /etc/yandex/@dir@/zoo.cfg
        fi
        ln -s $ENV_CONFIG /etc/yandex/@dir@/zoo.cfg

        ENV_REST_CONFIG=/etc/yandex/@dir@/rest.cfg.$ENV
        if test -f $ENV_REST_CONFIG ; then
            if [ -f /etc/yandex/@dir@/rest.cfg ]; then
                rm /etc/yandex/@dir@/rest.cfg
            fi
            ln -s $ENV_REST_CONFIG /etc/yandex/@dir@/rest.properties
        fi

        SLNG_CONFIG="/etc/syslog-ng/conf-enabled/@package@.conf"
        ENV_SLNG_CONFIG="/etc/syslog-ng/conf-available/@package@-$ENV.conf"
        if test -f $ENV_SLNG_CONFIG; then
            if [ -f $SLNG_CONFIG ]; then
                rm $SLNG_CONFIG
            fi
            ln -s $ENV_SLNG_CONFIG $SLNG_CONFIG
        fi

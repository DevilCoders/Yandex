#!/bin/sh -e
if [ -x /usr/local/java8/bin/java ]; then
	JAVA=/usr/local/java8/bin/java
elif [ -x /usr/local/java7/bin/java ]; then
        JAVA=/usr/local/java7/bin/java
elif [ -x /usr/bin/java ]; then
	JAVA=/usr/bin/java
else
	echo "No java found"
	exit 1
fi

echo "Java found: $JAVA"

exec $JAVA \
    -cp '/etc/yandex/@dir@:/usr/lib/yandex/@dir@/*:/usr/lib/yandex/@dir@/lib/*' \
    @javaNetworkOption@ \
    "$@"

# vim: set ts=4 sw=4 et:

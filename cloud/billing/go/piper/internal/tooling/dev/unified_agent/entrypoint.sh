#!/bin/sh

eval "echo \"$(cat /etc/yandex/unified_agent/config.tmpl.yml)\"" >/etc/yandex/unified_agent/config.yml

exec "$@"

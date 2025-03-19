#!/usr/bin/env sh
/bin/sed 's/JAEGER_COLLECTOR_PLACEHOLDER/${jaeger-endpoint}/g' /etc/jaeger-agent/jaeger-agent-config.yaml.tpl > /etc/jaeger-agent/jaeger-agent-config.yaml

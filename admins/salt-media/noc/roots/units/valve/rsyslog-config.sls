/etc/rsyslog.d/20-nginx-to-unified-agent.conf:
  file.managed:
    - makedirs: True
    - contents: |
        module(load="imfile")
        ## input files
        input (type="imfile" file="/var/log/nginx/tskv.log" tag="nginx" ruleset="fwd")
        ## output fwd
        ruleset(name="fwd") {
          action(type="omfwd" target="localhost" port="11514" protocol="udp")
        }

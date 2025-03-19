cleanup default configs:
  file.absent:
    - names:
      - /etc/rsyslog.d/50-default.conf
      - /etc/rsyslog.d/50-default.conf.ucf-dist
      - /etc/rsyslog.d/20-ufw.conf
      - /etc/rsyslog.d/21-cloudinit.conf

/etc/rsyslog.d/20-yanet-logs-to-unified-agent.conf:
  file.managed:
    - makedirs: True
    - contents: |
        # journald import
        module(load="imjournal" StateFile="imjournal" Ratelimit.Interval="0" IgnorePreviousMessages="on")
        module(load="mmjsonparse")

        template(name="date-rfc3339" type="string" string="%timegenerated:::date-rfc3339")
        template(name="yanet-json-log-fmt" type="string" string="%$!%\n")

        if ($!_SYSTEMD_UNIT startswith "vasgen") then {
          call yanet-logs
        } else if ($!_SYSTEMD_UNIT startswith "yanet") then {
          call yanet-logs
        } else if ($!_SYSTEMD_UNIT startswith "yadecap") then {
          call yanet-logs
        } else if ($!_SYSTEMD_UNIT startswith "yandex-bird") then {
          call yanet-logs
        }
        ruleset(name="yanet-logs") {
          # preserve meta
          set $._PID = $!_PID;
          set $._SYSTEMD_UNIT = $!_SYSTEMD_UNIT;
          set $._CMDLINE = $!_CMDLINE;
          set $._UID = $!_UID;
          unset $!;

          # set defaults
          set $!ts = exec_template("date-rfc3339");
          set $!level = $pri-text;
          set $!host = $hostname;

          action(type="mmjsonparse" cookie="")
          set $! = $.;  # enrich message by meta

          action(type="omfwd" target="localhost" port="10514" protocol="udp" template="yanet-json-log-fmt")
        }


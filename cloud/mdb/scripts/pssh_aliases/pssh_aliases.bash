# -*- mode : shell-script -*-

PSSH_SALT_CALL_TIMEOUT=60m

pssh-run() {
    options="$1"; shift;
    command="$1"; shift;
    pssh run $options "$command" $(echo "$@" | sed "s/,/ /g")
}


# System
pssh-service-status()                 { service="$1"; shift; pssh-run "-a -p 50" "sudo service $service status" "$@"; }
pssh-service-restart()                { service="$1"; shift; pssh-run "-a"       "sudo service $service restart" "$@"; }
pssh-service-restart-p()              { service="$1"; shift; pssh-run "-a -p 50" "sudo service $service restart" "$@"; }
pssh-service-start()                  { service="$1"; shift; pssh-run "-a"       "sudo service $service start" "$@"; }
pssh-service-start-p()                { service="$1"; shift; pssh-run "-a -p 50" "sudo service $service start" "$@"; }
pssh-service-stop()                   { service="$1"; shift; pssh-run "-a"       "sudo service $service stop" "$@"; }
pssh-service-stop-p()                 { service="$1"; shift; pssh-run "-a -p 50" "sudo service $service stop" "$@"; }
pssh-service-uptime()                 { service="$1"; shift; pssh-run "-a -p 50" "systemctl show --property ExecMainStartTimestamp --value $service" "$@"; }
pssh-service-restart-if-uptime-1d()   { service="$1"; shift; pssh-run "-a"       "start_ts=\$(date -d \"\$(systemctl show --property ExecMainStartTimestamp --value $service)\" +%s); uptime=\$(((\$(date +%s) - \$start_ts)/3600/24)); if [[ \$? -ne 0 ]] || [[ \$uptime -eq 0 ]]; then exit 0; fi; sudo service $service restart && echo restarted" "$@"; }
pssh-service-restart-if-uptime-1d-p() { service="$1"; shift; pssh-run "-a"       "start_ts=\$(date -d \"\$(systemctl show --property ExecMainStartTimestamp --value $service)\" +%s); uptime=\$(((\$(date +%s) - \$start_ts)/3600/24)); if [[ \$? -ne 0 ]] || [[ \$uptime -eq 0 ]]; then exit 0; fi; sudo service $service restart && echo restarted" "$@"; }

pssh-logs-cat()                       { file="$1"; shift; pssh-run "-a -p 50" "cat $file | xargs -n 1 -d '\n' echo -e" "$@"; }
pssh-logs-pipe()                      { file="$1"; shift; command="$1"; shift; pssh-run "-a -p 50" "cat $file | $command | xargs -n 1 -d '\n' echo -e || true" "$@"; }
pssh-logs-grep()                      { file_pattern="$1"; shift; pattern="$1"; shift; pssh-run "-a -p 50" "grep -niI $pattern $file_pattern | xargs -n 1 -d '\n' echo -e || true" "$@"; }
pssh-logs-zgrep()                     { file_pattern="$1"; shift; pattern="$1"; shift; pssh-run "-a -p 50" "zgrep -niI $pattern $file_pattern | xargs -n 1 -d '\n' echo -e || true" "$@"; }

pssh-grep-oom()                       { pssh-logs-grep "/var/log/syslog" "oom_reaper" "$@"; }
pssh-zgrep-oom()                      { pssh-logs-zgrep "/var/log/syslog" "oom_reaper" "$@"; }

pssh-ferm-configs()                   { pssh-run "-a" "find /etc/ferm/ -type f | xargs -n 1 -t cat 2>&1" "$@"; }
pssh-ferm-reload()                    { pssh-run "-a" "sudo service ferm reload" "$@"; }

pssh-apparmor-version()               { pssh-run "-a -p 50" "dpkg -l | fgrep apparmor" "$@"; }

pssh-dpkg-configure-a()               { pssh-run "-a -p 50" "dpkg --configure -a" "$@"; }

pssh-ensure-no-primary()              { pssh-run "-a" "/usr/local/yandex/ensure_no_primary.sh" "$@"; }


# Salt
pssh-parametrized-hs()                { pssh-parametrized-hs-if "" "$@"; }
pssh-parametrized-hs-if()             { command="$1"; shift; parameters="$1"; shift; pssh-run "" "$command set -o pipefail; timeout $PSSH_SALT_CALL_TIMEOUT salt-call --retcode-passthrough saltutil.sync_all 2>&1 && timeout $PSSH_SALT_CALL_TIMEOUT salt-call --retcode-passthrough --out highstate --output-diff state.highstate queue=True $parameters 2>&1 | tee highstate.log | grep -vP 'jinja_trim_blocks is deprecated|DeprecationWarning|^\\s*\$' || echo FAILED" "$@"; }
pssh-hs()                             { pssh-parametrized-hs "--log-file-level=info" "$@"; }
pssh-hs-p()                           { pssh-parametrized-hs "--log-file-level=info" "-p 50" "$@"; }
pssh-hs-if-healthy-p()                { pssh-parametrized-hs-if "monrun -c | grep -q -vP '^Type:|No checks|pushclient|load_average|backup|replication_lag|log_errors|restart_required|mdb_metrics'; if [[ \$? -eq 0 ]]; then echo 'Host is skipped as there are failed monrun checks'; monrun -c; exit 0; fi;" "--log-file-level=info" "-p 50" "$@"; }
pssh-hs-test()                        { pssh-parametrized-hs "--log-file-level=info test=True" "-p 50" "$@"; }
pssh-hs-debug()                       { pssh-parametrized-hs "--log-file-level=debug" "$@"; }
pssh-hs-with-restart()                { pssh-parametrized-hs "--log-file-level=info pillar='{service-restart: true}'" "" "$@"; }
pssh-hs-with-restart-p()              { pssh-parametrized-hs "--log-file-level=info pillar='{service-restart: true}'" "-p 50" "$@"; }
pssh-hs-with-restart-debug()          { pssh-parametrized-hs "--log-file-level=debug pillar='{service-restart: true}'" "" "$@"; }
pssh-hs-qa()                          { pssh-parametrized-hs "--log-file-level=info pillar='{yandex: {environment: qa}}'" "" "$@"; }
pssh-hs-qa-test()                     { pssh-parametrized-hs "--log-file-level=info test=True pillar='{yandex: {environment: qa}}'" "-p 50" "$@"; }
pssh-hs-if-ch-cluster()               { pssh-parametrized-hs-if "if ! grep -q clickhouse_cluster /etc/dbaas.conf 2>/dev/null; then echo 'Host is skipped as it does not belong to ClickHouse cluster' && exit 0; fi;" "--log-file-level=info" "" "$@"; }
pssh-hs-if-ch-cluster-p()             { pssh-parametrized-hs-if "if ! grep -q clickhouse_cluster /etc/dbaas.conf 2>/dev/null; then echo 'Host is skipped as it does not belong to ClickHouse cluster' && exit 0; fi;" "--log-file-level=info" "-p 50" "$@"; }
pssh-hs-if-kafka-cluster()            { pssh-parametrized-hs-if "if ! grep -q kafka_cluster /etc/dbaas.conf 2>/dev/null; then echo 'Host is skipped as it does not belong to Kafka cluster' && exit 0; fi;" "--log-file-level=info" "" "$@"; }
pssh-hs-if-kafka-cluster-p()          { pssh-parametrized-hs-if "if ! grep -q kafka_cluster /etc/dbaas.conf 2>/dev/null; then echo 'Host is skipped as it does not belong to Kafka cluster' && exit 0; fi;" "--log-file-level=info" "-p 50" "$@"; }
pssh-hs-container()                   { container="$1"; shift; pssh-parametrized-hs "pillar=\"{target-container: $container}\"" "" "$@"; }
pssh-hs-container-test()              { container="$1"; shift; pssh-parametrized-hs "pillar=\"{target-container: $container}\" test=True" "" "$@"; }
pssh-pillar-get()                     { value="$1"; shift; pssh-run "" "salt-call --output newline_values_only pillar.get $value" "$@"; }
pssh-get-environment()                { pssh-pillar-get "yandex:environment" "$@"; }
pssh-get-cluster-id()                 { pssh-pillar-get "data:dbaas:cluster_id" "$@"; }
pssh-get-shard-id()                   { pssh-pillar-get "data:dbaas:shard_id" "$@"; }
pssh-get-shard-name()                 { pssh-pillar-get "data:dbaas:shard_name" "$@"; }
pssh-grains-get()                     { value="$1"; shift; pssh-run "" "salt-call --output newline_values_only grains.get $value" "$@"; }
pssh-get-dc()                         { pssh-grains-get "ya:short_dc" "$@"; }


# Dom0
pssh-portoctl-list()                  { pssh-run "-p 50" "portoctl list" "$@"; }
pssh-portoctl-list-pipe()             { command="$1"; shift; pssh-run "-a -p 50" "portoctl list | $command || true" "$@"; }
pssh-portoctl-destroy()               { container="$1"; shift; pssh-run "-p 50" "portoctl destroy $container" "$@"; }


# Monrun
pssh-monrun()                         { pssh-run "-a -p 50" "sudo -u monitor monrun" "$@"; }
pssh-monrun-w()                       { pssh-run "-a -p 50" "sudo -u monitor monrun -w" "$@"; }
pssh-monrun-check-w()                 { pssh-run "-a -p 50" 'checks=$(monrun -w | grep -vP "(resps|salt_minion_status|restart_required)" | grep -oP "[\\w-]+ =" | awk "{print \$1}"); for check in $checks; do sudo -u monitor monrun -r $check | grep -v OK; done; monrun -w | grep salt_minion_status || true' "$@"; }
pssh-monrun-check-w-if-ch-cluster()   { pssh-run "-a -p 50" 'if ! grep -q clickhouse_cluster /etc/dbaas.conf 2>/dev/null; then echo "Host is skipped as it does not belong to ClickHouse cluster" && exit 0; fi; checks=$(monrun -w | grep -vP "(resps|salt_minion_status|restart_required)" | grep -oP "[\\w-]+ =" | awk "{print \$1}"); for check in $checks; do sudo -u monitor monrun -r $check | grep -v OK; done; monrun -w | grep salt_minion_status || true' "$@"; }
pssh-monrun-check-w-if-kafka-cluster(){ pssh-run "-a -p 50" 'if ! grep -q kafka_cluster /etc/dbaas.conf 2>/dev/null; then echo "Host is skipped as it does not belong to Kafka cluster" && exit 0; fi; checks=$(monrun -w | grep -vP "(resps|salt_minion_status|restart_required)" | grep -oP "[\\w-]+ =" | awk "{print \$1}"); for check in $checks; do sudo -u monitor monrun -r $check | grep -v OK; done; monrun -w | grep salt_minion_status || true' "$@"; }
pssh-monrun-disable-check()           { check="$1"; shift; pssh-run "-a -p 50" "sed -i -E -e '/^command/ s/=.*/= echo \"0;OK\"/' /etc/monrun/conf.d/${check}.conf && sudo -u monitor monrun --gen-jobs && sudo service juggler-client restart" "$@"; }
pssh-errata-update()                  { pssh-run "-a -p 50" /usr/local/sbin/errata_apply.sh "$@"; }


# MDB Metrics
pssh-metrics-version()                { pssh-run "-a -p 50" "dpkg-query --show yandex-mdb-metrics" "$@"; }
pssh-metrics-logs()                   { pssh-run "-p 50"    "cat /var/log/mdb-metrics.log" "$@"; }
pssh-metrics-logs-50()                { pssh-run "-p 50"    "cat /var/log/mdb-metrics.log | tail -50" "$@"; }
pssh-metrics-logs-pipe()              { command="$1"; shift; pssh-run "-p 50" "cat /var/log/mdb-metrics.log | $command || true" "$@"; }
pssh-metrics-warnings()               { pssh-run "-p 50"    "grep 'ERROR\|WARNING' /var/log/mdb-metrics.log" "$@"; }
pssh-metrics-warnings-50()            { pssh-run "-p 50"    "grep 'ERROR\|WARNING' /var/log/mdb-metrics.log | tail -50" "$@"; }
pssh-metrics-warnings-pipe()          { command="$1"; shift; pssh-run "-p 50" "grep 'ERROR\|WARNING' /var/log/mdb-metrics.log | $command" "$@"; }
pssh-metrics-errors()                 { pssh-run "-p 50"    "grep 'ERROR' /var/log/mdb-metrics.log" "$@"; }
pssh-metrics-errors-50()              { pssh-run "-p 50"    "grep 'ERROR' /var/log/mdb-metrics.log | tail -50" "$@"; }
pssh-metrics-errors-pipe()            { command="$1"; shift; pssh-run "-p 50" "grep 'ERROR' /var/log/mdb-metrics.log | $command" "$@"; }
pssh-metrics-find-sla-check-dt()      { pssh-run "-a -p 50" "find /tmp -name admin-marked-as-broken; find /var/run/lock -name instance_userfault_broken" "$@"; }
pssh-metrics-downtime-sla-check()     { pssh-run "-a -p 50" "set -x; touch /tmp/admin-marked-as-broken" "$@"; }
pssh-metrics-clean-sla-check-dt()     { pssh-run "-a -p 50" "rm -rfv /tmp/admin-marked-as-broken /var/run/lock/instance_userfault_broken" "$@"; }


# MDB SSH keys
pssh-mdb-keys-version()               { pssh-run "-a -p 50" "dpkg-query --show mdb-ssh-keys" "$@"; }
pssh-mdb-keys-update()                { version="$1"; shift; pssh-run "-a -p 50" "apt-get update -qq && apt-get install -y mdb-ssh-keys=${version}" "$@"; }


# Juggler
pssh-juggler-status()                 { pssh-service-status    "juggler-client" "$@"; }
pssh-juggler-restart()                { pssh-service-restart   "juggler-client" "$@"; }
pssh-juggler-restart-p()              { pssh-service-restart-p "juggler-client" "$@"; }


# MDB Health
pssh-health-ch-logs()                 { pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" || true" "$@"; }
pssh-health-ch-logs-50()              { pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | tail -50 || true" "$@"; }
pssh-health-ch-logs-pipe()            { command="$1"; shift; pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | $command || true" "$@"; }
pssh-health-ch-sla-logs()             { pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | fgrep 'under_sla\":true' || true" "$@"; }
pssh-health-ch-sla-logs-50()          { pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | fgrep 'under_sla\":true' | tail -50 || true" "$@"; }
pssh-health-ch-sla-logs-pipe()        { command="$1"; shift; pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | fgrep 'under_sla\":true' | $command || true" "$@"; }
pssh-health-ch-sla-logs-without-dt()  { pssh-run "-p 50" "fgrep clickhouse /var/log/supervisor/mdb-health.log | fgrep \"\$(date '+%Y-%m-%d')\" | fgrep 'under_sla\":true' | fgrep -v 'broken by usr' || true" "$@"; }


# MDB Maintenance
pssh-mdbm-version()                   { pssh-run "-a -p 50" "dpkg -l | fgrep mdb-maintenance" "$@"; }
pssh-mdbm-sync()                      { pssh-run "-a -p 50" "yazk-flock -c /etc/yandex/mdb-maintenance/zk-flock.json lock /etc/cron.yandex/mdb-maintenance-sync.sh || rm /var/run/mdb-maintenance-sync/last-exit-status" "$@"; }
pssh-mdbm-task-configs()              { pssh-run "-a -p 50" "ls -l /etc/yandex/mdb-maintenance/configs/" "$@"; }
pssh-mdbm-logs()                      { pssh-run "-a -p 50" "cat /var/log/mdb-maintenance/mdb-maintenance-sync.log" "$@"; }
pssh-mdbm-logs-pipe()                 { command="$1"; shift; pssh-run "-a -p 50" "cat /var/log/mdb-maintenance/mdb-maintenance-sync.log | $command" "$@"; }
pssh-mdbm-info-logs()                 { pssh-run "-a -p 50" "grep 'INFO\|WARN\|ERROR\|FATAL' /var/log/mdb-maintenance/mdb-maintenance-sync.log" "$@"; }
pssh-mdbm-warnings()                  { pssh-run "-a -p 50" "grep 'WARN\|ERROR\|FATAL' /var/log/mdb-maintenance/mdb-maintenance-sync.log" "$@"; }
pssh-mdbm-errors()                    { pssh-run "-a -p 50" "grep 'ERROR\|FATAL' /var/log/mdb-maintenance/mdb-maintenance-sync.log" "$@"; }
pssh-mdbm-fatals()                    { pssh-run "-a -p 50" "fgrep FATAL /var/log/mdb-maintenance/mdb-maintenance-sync.log" "$@"; }


# MDB Worker
pssh-worker-config()                  { pssh-run "-a -p 50" "cat /etc/dbaas-worker.conf" "$@"; }
pssh-worker-version()                 { pssh-run "-a -p 50" "dpkg-query --show dbaas-worker" "$@"; }
pssh-worker-fix-ext-dns()             { host="$1"; shift;    pssh-run "" "/opt/yandex/dbaas-worker/bin/fix-ext-dns $host" "$@"; }
pssh-worker-fix-service-account()     { cluster="$1"; shift; pssh-run "" "/opt/yandex/dbaas-worker/bin/fix-service-account $cluster" "$@"; }


# MDB Internal API
pssh-mdb-api-config()                  { pssh-run "" "cat /etc/yandex/mdb-internal-api/mdb-internal-api.yaml" "$@"; }
pssh-mdb-api-version()                 { pssh-run "-a -p 50" "dpkg-query --show mdb-internal-api" "$@"; }
pssh-mdb-api-logs()                    { pssh-logs-cat "/var/log/mdb-internal-api/api.log" "$@"; }
pssh-mdb-api-logs-pipe()               { pssh-logs-pipe "/var/log/mdb-internal-api/api.log" "$@"; }
pssh-mdb-api-errors()                  { pssh-logs-pipe "/var/log/mdb-internal-api/api.log" "grep ERROR" "$@"; }
pssh-mdb-api-errors-pipe()             { command="$1"; shift; pssh-logs-pipe "/var/log/mdb-internal-api/api.log" "grep ERROR | $command" "$@"; }
pssh-mdb-api-grep-request()            { request="$1"; shift; pssh-logs-pipe "/var/log/mdb-internal-api/api.log" "grep $request" "$@"; }


# ClickHouse
pssh-ch-query()                       { query="$1"; shift; pssh-run ""      "echo \"$query\" | clickhouse-client" "$@"; }
pssh-ch-query-p()                     { query="$1"; shift; pssh-run "-p 50" "echo \"$query\" | clickhouse-client" "$@"; }
pssh-ch-version()                     { pssh-run "-a -p 50" "echo 'SELECT version()' | clickhouse-client" "$@"; }
pssh-ch-deb-version()                 { pssh-run "-a -p 50" "dpkg-query --show clickhouse-server" "$@"; }
pssh-ch-upgrade()                     { pssh-run "" "current=\"\$(dpkg-query -f '\${Version}' -W clickhouse-server)\" target=\"\$(salt-call --output newline_values_only mdb_clickhouse.version)\"; if [[ \$current == \$target ]]; then echo ClickHouse version is up-to-date; exit 0; fi; set -o pipefail; timeout $PSSH_SALT_CALL_TIMEOUT salt-call --retcode-passthrough saltutil.sync_all 2>&1 && timeout $PSSH_SALT_CALL_TIMEOUT salt-call --retcode-passthrough state.highstate queue=True --log-file-level=info --output-diff pillar='{service-restart: true}' 2>&1 | tee highstate.log | grep -vP 'jinja_trim_blocks is deprecated|DeprecationWarning|^\\s*\$' || echo FAILED" "$@"; }
pssh-ch-mon()                         { pssh-run "-a -p 50" "ch-monitoring status" "$@"; }
pssh-ch-mon-w()                       { pssh-run "-a -p 50" "ch-monitoring status | grep -v OK" "$@"; }
pssh-ch-uptime()                      { pssh-run "-a"       "echo SELECT \"concat(toString(floor(uptime()/3600/24)), ' days ', toString(floor(uptime() % (24 * 3600) / 3600, 1)), ' hours')\" | clickhouse-client" "$@"; }
pssh-ch-restart()                     { pssh-run "" "sudo service clickhouse-server restart && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done" "$@"; }
pssh-ch-restart-if-uptime-1h()        { pssh-run "" "uptime=\$(clickhouse-client -q 'select toInt32(uptime()/3600)'); if [[ \$? -ne 0 ]] || [[ \$uptime -eq 0 ]]; then exit 0; fi; sudo service clickhouse-server restart && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done; echo restarted" "$@"; }
pssh-ch-restart-if-uptime-1d()        { pssh-run "" "uptime=\$(clickhouse-client -q 'select toInt32(uptime()/3600/24)'); if [[ \$? -ne 0 ]] || [[ \$uptime -eq 0 ]]; then exit 0; fi; sudo service clickhouse-server restart && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done; echo restarted" "$@"; }
pssh-ch-restart-with-tmp-cleanup()    { pssh-run "" "sudo service clickhouse-server stop && (find -L /var/lib/clickhouse/data -mindepth 3 -type d -name 'tmp_*' | xargs -r -n 1 -t rm -rf) && sudo service clickhouse-server start && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done" "$@"; }
pssh-ch-restart-with-tmp-cleanup-if-uptime-1d()    { pssh-run "" "uptime=\$(clickhouse-client -q 'select toInt32(uptime()/3600/24)'); if [[ \$? -ne 0 ]] || [[ \$uptime -eq 0 ]]; then exit 0; fi; sudo service clickhouse-server stop && (find -L /var/lib/clickhouse/data -mindepth 3 -type d -name 'tmp_*' | xargs -r -n 1 -t rm -rf) && sudo service clickhouse-server start && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done" "$@"; }
pssh-ch-start()                       { pssh-run "" "sudo service clickhouse-server start && while ! clickhouse-client -q 'SELECT 1' >/dev/null 2>&1; do sleep 1; done" "$@"; }
pssh-ch-stop()                        { pssh-run "" "sudo service clickhouse-server stop" "$@"; }
pssh-ch-errors()                      { pssh-run "-p 50" "fgrep '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log" "$@"; }
pssh-ch-errors-5()                    { pssh-run "-p 50" "fgrep '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log | tail -5" "$@"; }
pssh-ch-errors-10()                   { pssh-run "-p 50" "fgrep '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log | tail -10" "$@"; }
pssh-ch-errors-50()                   { pssh-run "-p 50" "fgrep '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log | tail -50" "$@"; }
pssh-ch-errors-pipe()                 { command="$1"; shift; pssh-run "-p 50" "fgrep '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log | $command" "$@"; }
pssh-ch-zgrep-errors-pipe()           { command="$1"; shift; pssh-run "-a -p 50" "zgrep -F '<Error>' /var/log/clickhouse-server/clickhouse-server.err.log* | $command" "$@"; }
pssh-ch-grep-restarts()               { pssh-run "-a -p 50" "grep -P 'Received termination signal|Starting (daemon|ClickHouse)|Ready for connections' /var/log/clickhouse-server/clickhouse-server.log || true" "$@"; }
pssh-ch-grep-restarts-pipe()          { command="$1"; shift; pssh-run "-a -p 50" "grep -P 'Received termination signal|Starting (daemon|ClickHouse)|Ready for connections' /var/log/clickhouse-server/clickhouse-server.log | $command" "$@"; }
pssh-ch-zgrep-restarts()              { pssh-run "-a -p 50" "zgrep -P 'Received termination signal|Starting (daemon|ClickHouse)|Ready for connections' /var/log/clickhouse-server/clickhouse-server.log* || true" "$@"; }
pssh-ch-zgrep-restarts-pipe()         { command="$1"; shift; pssh-run "-a -p 50" "zgrep -P 'Received termination signal|Starting (daemon|ClickHouse)|Ready for connections' /var/log/clickhouse-server/clickhouse-server.log* | $command" "$@"; }
pssh-ch-repl-queue-size()             { pssh-run "-a -p 50" "echo \"SELECT count(), countIf(last_exception != '') FROM system.replication_queue\" | clickhouse-client" "$@"; }
pssh-ch-repl-queue-exceptions()       { pssh-run "-a -p 50" "chadmin replication-queue list --failed" "$@"; }
pssh-ch-repl-lag()                    { pssh-run "-a -p 50" "ch-monitoring replication-lag -vvv" "$@"; }
pssh-ch-find-detached-data()          { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data -mindepth 4 -maxdepth 4 -wholename '*/detached/*' | xargs -n 1 -r du -hs" "$@"; }
pssh-ch-rm-detached-data()            { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data -mindepth 4 -maxdepth 4 -wholename '*/detached/*' | xargs -n 1 -r -t rm -rf" "$@"; }
pssh-ch-find-tmp-files()              { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data -mindepth 3 -type d -name 'tmp_*' 2>/dev/null || true" "$@"; }
pssh-ch-find-old-tmp-files()          { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data -mindepth 3 -type d -ctime +24 2>/dev/null | grep 'tmp[^/]*$' || true" "$@"; }
pssh-ch-rm-old-tmp-files()            { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data -mindepth 3 -type d -ctime +24 2>/dev/null | grep 'tmp[^/]*$' | xargs -r -n 1 -t rm -rf" "$@"; }
pssh-ch-find-old-system-parts()       { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data/system/ -mindepth 2 -maxdepth 2 -type d -ctime +10 | grep -v 'detached$'" "$@"; }
pssh-ch-rm-old-system-parts()         { pssh-run "-a -p 50" "find -L /var/lib/clickhouse/data/system/ -mindepth 2 -maxdepth 2 -type d -ctime +10 | grep -v 'detached$' | xargs -r -n 1 -t rm -rf" "$@"; }
pssh-ch-find-s3-broken-data()         { pssh-run "-a -p 50" "find /var/lib/clickhouse/disks/object_storage/ -size 0 | grep -v tmp || true" "$@"; }
pssh-ch-rm-s3-broken-data()           { pssh-run "-a -p 50" "find /var/lib/clickhouse/disks/object_storage/ -size 0 | grep -v tmp | xargs -n 1 -r -t rm -rf" "$@"; }
pssh-ch-find-kafka-tables()           { pssh-run "-a"       "grep -RI -A 1 'Kafka' /var/lib/clickhouse/metadata/ || true" "$@"; }
pssh-ch-set-kafka-num-consumers()     { value="$1"; shift; pssh-run "" "grep -RlI 'kafka_num_consumers = ' /var/lib/clickhouse/metadata/ | xargs -n 1 -t -r sed -i -E -e 's/kafka_num_consumers = [[:alnum:]]+/kafka_num_consumers = $value/' 2>&1" "$@"; }
pssh-ch-grep-crashes()                { pssh-run "-a -p 50" "has_crash_log=\$(clickhouse-client -q \"select count() from system.tables where name = 'crash_log'\"); if [[ \$? -ne 0 ]] || [[ \$has_crash_log -eq 0 ]]; then exit 0; fi; has_yt_dictionaries=\$(clickhouse-client -q \"select count() from system.dictionaries where source = '/usr/lib/libclickhouse_dictionary_yt.so'\"); if [[ \$? -ne 0 ]] || [[ \$has_yt_dictionaries -ne 0 ]]; then exit 0; fi; clickhouse-client -q 'select event_time, version, trace_full from system.crash_log'" "$@"; }
pssh-ch-config()                      { pssh-run "" "cat /etc/clickhouse-server/config.xml" "$@"; }
pssh-ch-storage-config()              { pssh-run "" "cat /etc/clickhouse-server/config.d/storage_policy.xml" "$@"; }
pssh-ch-preprocessed-config()         { pssh-run "" "cat /var/lib/clickhouse/preprocessed_configs/config.xml" "$@"; }
pssh-ch-set-log-level-trace()         { pssh-run "-a -p 50" "sed -i -E -e '/<level>/ s/>[[:alpha:]]+</>trace</' /etc/clickhouse-server/config.xml" "$@"; }
pssh-ch-set-log-level-debug()         { pssh-run "-a -p 50" "sed -i -E -e '/<level>/ s/>[[:alpha:]]+</>debug</' /etc/clickhouse-server/config.xml" "$@"; }
pssh-ch-set-log-level-info()          { pssh-run "-a -p 50" "sed -i -E -e '/<level>/ s/>[[:alpha:]]+</>information</' /etc/clickhouse-server/config.xml" "$@"; }
pssh-ch-set-max-memory-usage()        { value="$1"; shift; pssh-run "" "sed -i -E -e '/<max_server_memory_usage>/ s/>[[:alnum:]]+</>${value}</' /etc/clickhouse-server/config.xml" "$@"; }
pssh-ch-set-move-factor()             { value="$1"; shift; pssh-run "" "sed -i -E -e '/<move_factor>/ s/>[[:alnum:].]+</>${value}</' /etc/clickhouse-server/config.d/storage_policy.xml" "$@"; }
pssh-ch-tools-version()               { pssh-run "-a -p 50" "dpkg-query --show mdb-ch-tools" "$@"; }
pssh-ch-tools-update()                { version="$1"; shift; pssh-run "-a -p 50" "apt-get update -qq && apt-get install -y mdb-ch-tools=${version}" "$@"; }
pssh-ch-has-datetime-columns()        { pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"select count() from system.columns where type like '%DateTime%'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }
pssh-ch-has-decimal-columns()         { pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"select count() from system.columns where type like '%Decimal%'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }
pssh-ch-has-decimal128-columns()      { pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"select count() from system.columns where type like '%Decimal128%'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }
pssh-ch-has-ipv6-in-schema()          { pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"select count() from system.tables where database != 'system' and create_table_query like '%IPv6%'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }
pssh-ch-has-database()                { database="$1"; shift; pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"SELECT count() FROM system.databases WHERE database = '$database'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }
pssh-ch-has-database()                { database="$1"; shift; pssh-run "-a -p 50" "count=\$(clickhouse-client -q \"SELECT count() FROM system.databases WHERE database = '$database'\"); if [[ \$? -ne 0 ]] || [[ \$count -eq 0 ]]; then echo false; exit 0; fi; echo true" "$@"; }


# ClickHouse Backups
pssh-ch-backup-version()              { pssh-run "-a -p 50" "dpkg-query --show ch-backup" "$@"; }
pssh-ch-backup-update()               { version="$1"; shift; pssh-run "-a -p 50" "apt-get update -qq && apt-get install -y ch-backup=${version}" "$@"; }
pssh-ch-backup-find()                 { pssh-run "-a -p 50" "pgrep -f 'bin/ch-backup' -a | grep -v pgrep || true" "$@"; }
pssh-ch-backup-kill()                 { pssh-run "-a -p 50" "pgrep -f 'bin/ch-backup' -a | grep -v pgrep | awk {'print $1'} | xargs -r -t kill -9" "$@"; }
pssh-ch-backup-find-purge()           { pssh-run "-a -p 50" "pgrep -f 'bin/ch-backup purge' -a | grep -v pgrep || true" "$@"; }
pssh-ch-backup-kill-purge()           { pssh-run "-a -p 50" "pgrep -f 'bin/ch-backup purge' -a | grep -v pgrep | awk {'print $1'} | xargs -r -t kill -9" "$@"; }
pssh-ch-backup-logs-pipe()            { command="$1"; shift; pssh-run "-p 50" "cat /var/log/ch-backup/ch-backup.log | $command" "$@"; }
pssh-ch-backup-stderr-logs()          { pssh-run "-a -p 50" "zcat /var/log/ch-backup/stderr.log.1.gz 2>/dev/null; cat /var/log/ch-backup/stderr.log 2>/dev/null || true" "$@"; }
pssh-ch-backup-stderr-logs-pipe()     { command="$1"; shift; pssh-run "-a -p 50" "(zcat /var/log/ch-backup/stderr.log.1.gz 2>/dev/null; cat /var/log/ch-backup/stderr.log 2>/dev/null) | $command || true" "$@"; }
pssh-ch-backup-stdout-logs()          { pssh-run "-a -p 50" "zcat /var/log/ch-backup/stdout.log.1.gz 2>/dev/null; cat /var/log/ch-backup/stdout.log 2>/dev/null || true" "$@"; }
pssh-ch-backup-stdout-logs-pipe()     { command="$1"; shift; pssh-run "-a -p 50" "(zcat /var/log/ch-backup/stdout.log.1.gz 2>/dev/null; cat /var/log/ch-backup/stdout.log 2>/dev/null) | $command || true" "$@"; }
pssh-ch-backup-config()               { pssh-run "-a -p 50" "cat /etc/yandex/ch-backup/ch-backup.conf" "$@"; }
pssh-ch-backup-config-pipe()          { command="$1"; shift; pssh run -a -p 50 "cat /etc/yandex/ch-backup/ch-backup.conf | $command" "$@"; }
pssh-ch-backup-dedup-age-limit()      { pssh-run "-a -p 50" "grep -A 2 deduplication_age_limit /etc/yandex/ch-backup/ch-backup.conf" "$@"; }
pssh-ch-backup-dedup-age-limit-set()  { limit="$1"; shift; pssh run -a -p 50 "sed -i -e '5s/days:.*/days: $limit/' /etc/yandex/ch-backup/ch-backup.conf" "$@"; }
pssh-ch-backup-test-zk-flock()        { pssh-run "-a" 'zk-flock -c /etc/yandex/ch-backup/zk-flock.json lock "echo OK"' "$@"; }
pssh-ch-backup-disable-monrun-check() { pssh-run "-a -p 50" "sed -i -E -e '/^command/ s/=.*/= echo \"0;OK\"/' /etc/monrun/conf.d/ch_backup_age.conf && sudo -u monitor monrun --gen-jobs && sudo service juggler-client restart" "$@"; }
pssh-ch-s3backup-check()              { pssh-run "-a -p 50" "ch-monitoring orphaned-backups" "$@"; }
pssh-ch-s3backup-clean()              { pssh-run "" "chadmin chs3-backup cleanup" "$@"; }


# ZooKeeper
pssh-zk-version()                     { pssh-run "-a" "dpkg -l | fgrep 'zookeeper '" "$@"; }
pssh-zk-logs()                        { pssh-run ""   "cat /var/log/zookeeper/zookeeper.log" "$@"; }
pssh-zk-logs-pipe()                   { command="$1"; shift; pssh-run "" "cat /var/log/zookeeper/zookeeper.log | $command" "$@"; }
pssh-zk-errors()                      { pssh-run "" "grep -vP 'INFO|Exception causing close of session|Connection request from old client|Unable to read additional data from client' /var/log/zookeeper/zookeeper.log" "$@"; }
pssh-zk-errors-pipe()                 { command="$1"; shift; pssh-run "" "grep -vP 'INFO|Exception causing close of session|Connection request from old client|Unable to read additional data from client' /var/log/zookeeper/zookeeper.log | $command" "$@"; }
pssh-zk-grep-election-logs()          { pssh-run "" "grep Election /var/log/zookeeper/zookeeper.log" "$@"; }
pssh-zk-grep-election-logs-pipe()     { command="$1"; shift; pssh-run "" "grep Election /var/log/zookeeper/zookeeper.log | $command" "$@"; }
pssh-zk-grep-elections()              { pssh-run "" "grep -P 'New election|Created server|ELECTION TOOK' /var/log/zookeeper/zookeeper.log" "$@"; }
pssh-zk-grep-elections-pipe()         { command="$1"; shift; pssh-run "" "grep -P 'New election|Created server|ELECTION TOOK' /var/log/zookeeper/zookeeper.log | $command" "$@"; }
pssh-zk-grep-oom()                    { pssh-run "" "grep -B 1 'OutOfMemoryError' /var/log/zookeeper/zookeeper.log" "$@"; }
pssh-zk-grep-oom-pipe()               { command="$1"; shift; pssh-run "" "grep -B 1 'OutOfMemoryError' /var/log/zookeeper/zookeeper.log | $command" "$@"; }
pssh-zk-zgrep-errors()                { pssh-run "" "zgrep -vP 'INFO|Exception causing close of session|Connection request from old client|Unable to read additional data from client' /var/log/zookeeper/zookeeper.log*" "$@"; }
pssh-zk-zgrep-errors-pipe()           { command="$1"; shift; pssh-run "" "zgrep -vP 'INFO|Exception causing close of session|Connection request from old client|Unable to read additional data from client' /var/log/zookeeper/zookeeper.log* | $command" "$@"; }
pssh-zk-status()                      { pssh-run "-a -p 50" "echo \$(/usr/local/yandex/monitoring/zookeeper.py alive | sed -E -e 's/^[0-9]+;//') '' \$(echo mntr | nc localhost 2181 | grep zk_server_state | cut -f2)" "$@"; }
pssh-zk-mntr()                        { pssh-run ""      "echo mntr | nc localhost 2181" "$@"; }
pssh-zk-restart()                     { pssh-service-restart   "zookeeper" "$@"; }
pssh-zk-restart-p()                   { pssh-service-restart-p "zookeeper" "$@"; }
pssh-zk-start()                       { pssh-service-start     "zookeeper" "$@"; }
pssh-zk-start-p()                     { pssh-service-start-p   "zookeeper" "$@"; }
pssh-zk-stop()                        { pssh-service-stop      "zookeeper" "$@"; }
pssh-zk-stop-p()                      { pssh-service-stop-p    "zookeeper" "$@"; }


# PostgreSQL
pssh-pg-status()                      { pssh-run "-a -p 50" "cd /tmp; sudo -u postgres \$(echo ~postgres)/.role.sh" "$@"; }

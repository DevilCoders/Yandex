#!/bin/bash

LOCK_TIMEOUT_SEC=10
RETRY_DELAY_SEC=5
# Please keep this value consistent with NAMED_TIMEOUT_SEC in the contrail-dns check.
# Also, be careful not to exceed 300 seconds of the total execution time.
# Note (simonov-d): after single-view contrail-dns (3.2.3.127) is deployed everywhere, this timeout may be lowered to 30 seconds.
RNDC_TIMEOUT_SEC=60

RECONFIG_CMD="timeout $RNDC_TIMEOUT_SEC /usr/bin/contrail-rndc -c /etc/contrail/dns/contrail-rndc.conf reconfig"
RELOAD_CMD="timeout $RNDC_TIMEOUT_SEC /usr/bin/contrail-rndc -c /etc/contrail/dns/contrail-rndc.conf reload"
RESTART_CMD="systemctl restart contrail-named"
TIMERS_DIRECTORY="/var/run/yc/named-reconfig-timer"

# CLOUD-13065: Lock file which is acquired by contrail-dns while new configuration is generating
# This value should be equal to config_generation_lock_file option in contrail-dns.conf. 
# Default value is /var/lock/contrail-named-configuration.lock
LOCK_FILE="/var/lock/contrail-named-configuration.lock"

# Call via $(run_and_log_time $COMMAND $TIMER_FILE $TRY_INDEX)
function run_and_log_time() {
    echo $3 > $2;

    # CLOUD-13065: Acquire lock before reconfiguring contrail-named.
    touch $LOCK_FILE
    (
        flock --exclusive --wait $LOCK_TIMEOUT_SEC 200 || exit 1

        /usr/bin/time -f '%e' -o $2 -a --quiet $1
    ) 200>$LOCK_FILE
}

mkdir -p "$TIMERS_DIRECTORY"

rm -f "$TIMERS_DIRECTORY/reconfig"
rm -f "$TIMERS_DIRECTORY/reload"
rm -f "$TIMERS_DIRECTORY/restart"

for retry in {1..3}; do
    echo "Trying reconfig #$retry ..."
    if $(run_and_log_time "$RECONFIG_CMD" "$TIMERS_DIRECTORY/reconfig" $retry); then
        echo "Reconfig #$retry succeeded."
        exit 0
    fi
    echo "Reconfig #$retry failed. Sleep $RETRY_DELAY_SEC seconds..."
    sleep $RETRY_DELAY_SEC
done
    
echo "All reconfig attempts have failed. Trying to reload..."
    
if $(run_and_log_time "$RELOAD_CMD" "$TIMERS_DIRECTORY/reload" 1); then
    echo "Reload succeeded."
    exit 0
fi

echo "Reload failed. Trying to restart..."

run_and_log_time "$RESTART_CMD" "$TIMERS_DIRECTORY/restart" 1


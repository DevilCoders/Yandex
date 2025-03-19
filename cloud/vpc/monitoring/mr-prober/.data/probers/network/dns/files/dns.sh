#!/bin/bash -x

# TODO: It's a legacy from old monrun-style checks from yc-e2e-tests. It's not needed anymore, we should
# fully refactor this prober
success() {
    echo "[ SUCCESS ] $(date '+%F %X.%N') $1"
}

fail() {
    echo "[   FAIL  ] $(date '+%F %X.%N') $1"
    exit 1
}

# end common part

echo "[ STARTED ] $(date '+%F %X.%N')"

# By default, DNS queries go to eth1, which is IPv6 interface into Mr.Prober control network
# Here we get DNS server for eth0 (internal IPv4 interfaces), and send queries specially to it
DNS_SERVER="$(ifconfig eth0 | grep -oP "(?<=inet )\d+\.\d+\.")0.2"

DIG=$(which dig)
if [ -z "$DIG" ]; then
    fail "dig not found, can't run tests"
    exit 1  # If wrong usage, exit with non-zero status
fi

DIG="$DIG @$DNS_SERVER"

TEST_NAME=$1
if [ -z "$TEST_NAME" ]; then
    fail "test not specified"
    exit 1  # If wrong usage, exit with non-zero status
fi

e2e_hostname_resolve() {
    local tgt=${1:-github.com yandex.ru google.com mail.ru}
    for host in $tgt; do
        local dig_output
        local exit_code
        for rec_type in {A,AAAA}; do
            dig_output=$($DIG +short -t $rec_type "$host" 2>/dev/null)
            exit_code=$?
            if [ $exit_code -eq 0 ] && [ -n "$dig_output" ]; then
                break
            fi
        done

        if [ $exit_code -ne 0 ]; then
            fail "Unable to execute $DIG: error $exit_code"
            return 1
        fi

        # Check that dig returned at least one address
        if [ -z "$dig_output" ]; then
            fail "Unable to get DNS record for $host"
            return 2
        fi
    done

    return 0
}

e2e_dns64() {
    local tgt=${1:-github.com}

    local dig_output
    dig_output=$($DIG +short -t AAAA "$tgt" 2>/dev/null)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        fail "Unable to execute $DIG: error $exit_code"
        return 1
    fi

    # Check that dig returned at least one address
    if [ -z "$dig_output" ]; then
        fail "Unable to get AAAA DNS record for $tgt"
        return 2
    fi

    # Check if it is within the well-known prefix (RFC 6052)
    for AAAA in $dig_output; do
        if [[ "$AAAA" != 64:ff9b:* ]]; then
            fail "$AAAA is not within RFC 6052 well-known prefix"
            return 3
        fi
    done

    return 0
}

e2e_int_host() {
    local tgt=${1:-storage-int.mds.yandex.net}

    local dig_output
    dig_output=$($DIG +short -t AAAA "$tgt" 2>/dev/null)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        fail "Unable to execute $DIG: error $exit_code"
        return 1
    fi

    # Check that dig returned at least one address
    if [ -z "$dig_output" ]; then
        fail "Unable to get AAAA DNS record for $tgt"
        return 2
    fi

    # Check if it is within _YANDEXNETS_
    for AAAA in $dig_output; do
        if [[ "$AAAA" != 2a02:6b8:* ]]; then
            fail "$AAAA is not within _YANDEXNETS_"
            return 3
        fi
    done

    return 0
}

e2e_vm_record() {
    local tgt_name=$1
    local tgt_ip=$2
    local tgt_type=${3:-AAAA}

    if [ -z "$tgt_name" -o -z "$tgt_ip" ]; then
        fail "Either target name($tgt_name) or target_ip($tgt_ip) is empty"
        return 1
    fi

    local dig_output
    dig_output=$($DIG +short -t "$tgt_type" "$tgt_name" 2>/dev/null)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        fail "Unable to execute $DIG: error $exit_code"
        return 2
    fi

    # Check that dig returned at least one address
    if [ -z "$dig_output" ]; then
        fail "Got no $tgt_type records for $tgt_name"
        return 3
    fi

    # Check that IP address is present in the output
    for ip in $dig_output; do
        if [ "$ip" == "$tgt_ip" ]; then
            return 0
        fi
    done

    fail "Unable to find IP $tgt_ip in $tgt_type records for $tgt_name"
    return 4
}


e2e_vm_reverse_record() {
    local tgt_name=$1
    local tgt_ip=$2

    if [ -z "$tgt_name" -o -z "$tgt_ip" ]; then
        fail "Invalid test invocation"
        return 1
    fi

    local dig_output
    dig_output=$($DIG +short -x "$tgt_ip" 2>/dev/null)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        fail "Unable to execute $DIG: error $exit_code"
        return 2
    fi

    # Check that dig returned at least one address
    if [ -z "$dig_output" ]; then
        fail "Unable to get reverse DNS record for $tgt_ip"
        return 3
    fi

    # Check that hostname is present in the output
    for name in $dig_output; do
        if [ "$name" == "$tgt_name" -o "$name" == "$tgt_name." ]; then
            return 0
        fi
    done

    fail "Unable to find hostname $tgt_name in PTR records for $tgt_ip"
    return 4
}

run_test() {
    local testcase=${1:-/bin/false}
    shift

    $testcase "$@"
    if [ $? -eq 0 ]; then
        # Expect the test to log failure itself, but log success here
        success "passed $testcase $*"
    fi
}

shift
run_test "$TEST_NAME" "$@"

# Always success, fail() communicates errors via the protocol
exit 0

#!/bin/bash
# vim: set tabstop=4 shiftwidth=4 expandtab:
#
# This script up and stop SLB tunnel interfaces.
#
# More information look for in wiki:
# - https://wiki.yandex-team.ru/NOC/slb-TUN-RSSetup
#
# Relevant variables for /etc/rc.conf.local is next:
#   ya_slb_tunnel_enable            - if set to 'YES' up IPv4 SLB tunnel (default NO)
#   ya_slb6_tunnel_enable           - if set to 'YES' up IPv6 SLB tunnel (default NO)
#   ya_slb_tunnel_mtu               - MTU for route to SLB tunnel (default 1450)
#   ya_slb_tunnel_mtu_iface         - MTU for tunnel interfaces (default 8910)
#   ya_slb6_tunnel_ya_jumbo_enable  - if set to 'YES' add routes with jumbo frames (8910)
#                                           from tunnel iface to IPv6 networks (2a02:6b8::/32)
#
# So, you may also specify this options in '/etc/network/interfaces' for choosen interface.
# You should note, that specifed interfaces must be a default gateway.
# For example:
#   iface eth2 inet6 auto
#       privext 0
#       mtu 8950
#       ya-slb-tun YES              # the same as 'ya_slb_tunnel_enable'
#       ya-slb6-tun YES             # the same as 'ya_slb6_tunnel_enable'
#       ya-slb-tun-mtu 1450         # the same as 'ya_slb_tunnel_mtu'
#       ya-slb-tun-mtu-iface 8950   # the same as 'ya_slb_tunnel_mtu_iface'
#       ya-slb-tun-jumbo YES        # the same as 'ya_slb6_tunnel_ya_jumbo_enable'
#
# This script also save information about tunnels and state to '/run/network/ya-slb-tun' directory.
#

#
# ya_slb_tun_init_vars
#   Init basic variables.
#
ya_slb_tun_init_vars()
{
    # constants
    YA_SLB_TUN_RUNDIR="/run/network/ya-slb-tun"
    YA_SLB_TUN_CACHEDIR="/var/cache/network/ya-slb-tun"
    YA_SLB_TUN_JUMBO_IPV6_NETWORKS='2a02:6b8::/32'
    YA_SLB_TUN_JUMBO_MTU="8910"
    YA_SLB_TUN_ROUTE_TABLE_IPV4="4010"
    YA_SLB_TUN_ROUTE_TABLE_IPV6="6010"
    YA_SLB_TUN_DUMMY="dummy0"


    local ya_slb_tunnel_enable="YES"
    local ya_slb6_tunnel_enable="YES"
    local ya_slb6_tunnel_ya_jumbo_enable="NO"
    local ya_slb_tunnel_mtu="1450"
    local ya_slb_tunnel_mtu_iface=${YA_SLB_TUN_JUMBO_MTU}
    local ya_slb_tunnel_mtu_ip4ip6=${YA_SLB_TUN_JUMBO_MTU}
    local ya_slb_tunnel_conf="${YA_SLB_TUN_RUNDIR}/slb_tunnel.conf"
    local ya_slb6_tunnel_conf="${YA_SLB_TUN_RUNDIR}/slb6_tunnel.conf"
    local ya_slb_tunnel_conf_common="${YA_SLB_TUN_RUNDIR}/slb_tunnel_common.conf"

    # this variables will be exported
    : ${IF_YA_SLB_TUN:=$ya_slb_tunnel_enable}
    : ${IF_YA_SLB6_TUN:=$ya_slb6_tunnel_enable}
    : ${IF_YA_SLB_TUN_MTU:=$ya_slb_tunnel_mtu}
    : ${IF_YA_SLB_TUN_MTU_IFACE:=$ya_slb_tunnel_mtu_iface}
    : ${IF_YA_SLB_TUN_MTU_IP4IP6:=$ya_slb_tunnel_mtu_ip4ip6}
    : ${IF_YA_SLB6_TUN_JUMBO:=$ya_slb6_tunnel_ya_jumbo_enable}
    : ${IF_YA_SLB_TUN_CONF:=$ya_slb_tunnel_conf}
    : ${IF_YA_SLB6_TUN_CONF:=$ya_slb6_tunnel_conf}
    : ${IF_YA_SLB_TUN_CONF_COMMON:=$ya_slb_tunnel_conf_common}
    : ${IF_YA_SLB_TUN_STATE:="${YA_SLB_TUN_RUNDIR}/state"}
}

#
# ya_slb_tun_usage
#   Print usage information.
#
ya_slb_tun_usage()
{
    echo "$(basename $0) start|stop|restart|reload [interface]" >&2
}

#
# ya_check_enabled <variable_name>
#   Check that variable exists and it's value is 'yes', 'true', '1' or 'on'.
#   Return 0 if enabled, nonzero otherwise.
#
ya_check_enabled()
{
    local _value
    eval _value=\$${1}
    case ${_value} in
    # "yes", "true", "on", or "1"
    [Yy][Ee][Ss]|[Tt][Rr][Uu][Ee]|[Oo][Nn]|1)
        return 0
        ;;
    *)
        return 1
        ;;
    esac
}

#
# ya_log <message>
#   Print log message.
#
ya_log()
{
    echo "[$(date +%Y-%m-%d\ %H:%M:%S)] $@."
}

#
# ya_log_debug <message>
#   Print debug message.
#
ya_log_debug()
{
    ya_log "$@"
}

#
# ya_check_real_interface <interface>
#   Checks that interface is not loopback, tap, tun or other additional interface.
#
ya_check_real_interface()
{
    local _iface
    if [ ${#} -lt 1 -o -z "${1}" ] ; then
        ya_log_debug "ya_check_real_interface: no interface specified"
        return 2
    fi

    _iface=${1}
    # delete trailing numbers and downcase letters
    _iface="$(echo ${_iface%%[0-9]*} | tr '[:upper:]' '[:lower:]')"

    if [ -z ${_iface} ] ; then
         return 1
     fi

    case ${_iface} in
    lo|vlan|tun|ip6tnl|tap|vif|ipfw|pflog|virbr|plip|dummy)
        return 1
        ;;
    --all|all)
        return 1
        ;;
    esac

    return 0
}

#
# ya_get_active_interface <interface>
#   Get active interface:
#       - interface must be as a default gateway.
#       - interface must has a real name.
#
ya_get_active_interface()
{
    local _ifaces _iface
    _iface=${1:-''}

    _ifaces=""
    _ifaces=$(ip -4 route list exact 0.0.0.0/0 scope global | grep -v 'dev tun' | grep -oP 'dev\s+\K\w+')
    _ifaces="$_ifaces $(ip -6 route list exact ::/0 scope global | grep -oP 'dev\s+\K\w+')"
    _ifaces=$(echo $_ifaces | tr " " "\n" | uniq)

    if [ ! -z "$_iface" ] ; then
        _ifaces=$(echo $_ifaces | grep -o $_iface)
    fi

    for _iface in $_ifaces ; do
        if ya_check_real_interface $_iface ; then
            echo $_iface
            return
        fi
    done

    echo -n ''
    return
}

#
# ya_get_ip4addr <iface>
#   Print global IPv4 address of specified interface.
#   If address not found or interface specifed 'none' will be returned.
#
ya_get_ip4addr()
{
    local _iface _ip

    if [ ${#} -ne 1 ] ; then
        echo 'none'
        return 1
    fi

    _iface=${1}
    _ip=$(ip -o -4 addr show scope global dev ${_iface} |\
        awk '! / (10\.|192\.168).*/ {sub(/\/.*/, "", $4); print $4; exit}')

    if [ -z "${_ip}" ] ; then
        echo 'none'
        return 2
    fi

    echo "${_ip}"
    return 0
}

#
# ya_get_ip6addr <iface>
#   Print global IPv6 address of specified interface.
#   If address not found or interface specifed 'none' will be returned.
#
ya_get_ip6addr()
{
    local _iface _ip

    if [ ${#} -ne 1 ] ; then
        echo 'none'
        return 1
    fi

    _iface=${1}
    _ip=$(ip -o -6 addr show scope global dev ${_iface} |\
            awk '! / (fd|fc).*/ && ! /temporary/ {sub(/\/.*/, "", $4); print $4; exit}')

    if [ -z "${_ip}" ] ; then
        echo "none"
        return 2
    fi

    echo "${_ip}"
    return 0
}

#
# ya_is_ipv4 <ip>
#   Returns 0 if 'ip' is IPv4 address, otherwise returns 1.
#
ya_is_ipv4()
{
    if [ $# -ne 1 -o -z "$1" ] ; then
        return 1
    fi

    if [ -z "${1##*.*}" ] ; then
        return 0
    fi

    return 1
}

#
# ya_is_ipv6 <ip>
#   Returns 0 if 'ip' is IPv6 address, otherwise returns 1.
#
ya_is_ipv6()
{
    if [ $# -ne 1 -o -z "$1" ] ; then
        return 1
    fi

    if [ -z "${1##*:*}" ] ; then
        return 0
    fi

    return 1
}

#
# ya_slb_tun_is_configured
#   Check that SLB tun already configured.
#
ya_slb_tun_is_configured()
{
    if [ -f ${IF_YA_SLB_TUN_STATE} ] ; then
        cat ${IF_YA_SLB_TUN_STATE}
        return 0
    fi

    echo -n ''
    return 1
}

#
# ya_slb_tun_mark_configured
#   Set a flag, that ya-slb-tun configured.
#
ya_slb_tun_mark_configured()
{
    echo $@ > ${IF_YA_SLB_TUN_STATE}

    return 0
}

#
# ya_slb_tun_unmark_configured
#   Unset a flag, that ya-slb-tun configured.
#
ya_slb_tun_unmark_configured()
{
    rm -f ${IF_YA_SLB_TUN_STATE}

    return 0
}

#
# ya_slb_tun_fetcher <ip> <conf>
#   Fetch SLB configuragtion from Racktables API by IP to file.
#
ya_slb_tun_fetcher()
{
    local _ip _conf _fetch_args _wget_args _url _fetch_cmd _max_tries _cache

    if [ ${#} -ne 2 -o -z "${1}" -o -z "${2}" ] ; then
        ya_log_debug "ya_slb_tun_fetcher params error: 2 params required"
        return 1
    fi

    # initialize variables
    _ip="${1}"
    _conf="${2}"
    _cache="${YA_SLB_TUN_CACHEDIR}/$(basename ${_conf})"
    _url="https://ro.racktables.yandex.net/export/slb-info.php?mode=vips&ip=${_ip}"
    _fetch_args="-q -T 15 -o ${_conf}"
    _wget_args="-q -t 1 -T 15 -O ${_conf} --no-check-certificate"
    _max_tries=4
    if ! _fetch_cmd="$(which fetch) ${_fetch_args} ${_url}" ; then
        if ! _fetch_cmd="$(which wget) ${_wget_args} ${_url}" ; then
            ya_log "Can't find fetching command (fetch and wget doesn't exists)"
            return 3
        fi
    fi

    # make empty file
    echo -n > ${_conf}
    ya_log_debug "${_conf}"
    chmod 644 ${_conf}

    mkdir -p $(dirname ${_cache})

    # check that IP is specified
    if [ -z "$_ip" -o "$_ip" = "none" ] ; then
        ya_log "IP address for fetching SLB TUN information not found"
        return 2
    fi

    # try fetch file
    local i=0
    ya_log_debug "Fetching SLB configuration ${_fetch_cmd}"
    while [ ! ${i} -eq "${_max_tries}" ] ; do
        if ${_fetch_cmd} > /dev/null 2>&1 ; then
            # megakostyl by borislitv https://st.yandex-team.ru/TRAFFIC-10209
	    cp "/etc/network/l3_tun"  ${_conf}
            cp ${_conf} ${_cache}
            return 0
        fi

        i=$((${i} + 1))
        sleep ${i}
    done

    # if we can't fetch a file, we may use cached version.
    if [ -f ${_cache} ] ; then
        cp ${_cache} ${_conf}
    fi

    return 4
}

#
# ya_slb_tun_get_config <interface> [<config_name>]
#   Get common SLB configuration for <interface> and save it to <config_name>.
#
ya_slb_tun_get_config()
{
    local _iface _conf _ip4 _ip6

    if [ ${#} -lt 1 ] ; then
        ya_log_debug "ya_slb_tun_get_config expected minimum 1 params: '${#}' given"
        return 1
    fi

    _iface=${1}
    _conf=${2:-$IF_YA_SLB_TUN_CONF_COMMON}
    _ip4=$(ya_get_ip4addr ${_iface})
    _ip6=$(ya_get_ip6addr ${_iface})

    ya_slb_tun_fetcher ${_ip4} ${IF_YA_SLB_TUN_CONF}
    ya_slb_tun_fetcher ${_ip6} ${IF_YA_SLB6_TUN_CONF}
    cat ${IF_YA_SLB_TUN_CONF} ${IF_YA_SLB6_TUN_CONF} | sort | uniq > ${_conf}
    chmod 644 ${_conf}

    if [ "$(wc -l ${_conf})" != "0" ] ; then
        return 0
    fi

    return 2
}

#
# ya_slb_tun_add_tun_ip slb_tunnel.conf [dummy_interface]
#    Add SLB tunnel IP to dummy interface.
#
ya_slb_tun_add_tun_ip()
{
    local _conf _tun_ip _dummy_iface

    if [ ${#} -lt 1 -o -z "${1}" ] ; then
        ya_log_debug "ya_slb_tun_add_tun_ip: no configuration file specified"
        return 1
    fi

    _conf="${1}"
    if [ ! -e "${_conf}" ] ; then
        return 1
    fi

    _dummy_iface=${2:-$YA_SLB_TUN_DUMMY}
    while read _tun_ip ; do
        if ip addr show | egrep -q "\b${_tun_ip}[/ ]" ; then
            continue
        fi

        if ya_is_ipv4 $_tun_ip ; then
            if ! ya_check_enabled 'IF_YA_SLB_TUN' ; then
                continue
            fi

            ya_log_debug "Adding tunnel IP ${_tun_ip} to ${_dummy_iface}"
            ip addr add ${_tun_ip}/32 dev ${_dummy_iface}
            ya_log_debug "Adding rule ${YA_SLB_TUN_ROUTE_TABLE_IPV4} for ${_tun_ip}"
            ip rule add from ${_tun_ip} lookup ${YA_SLB_TUN_ROUTE_TABLE_IPV4}
        fi

        if ya_is_ipv6 $_tun_ip ; then
            if ! ya_check_enabled 'IF_YA_SLB6_TUN' ; then
                continue
            fi

            ya_log_debug "Adding tunnel IP ${_tun_ip} to ${_dummy_iface}"
            ip -6 addr add ${_tun_ip}/128 dev ${_dummy_iface}
            ya_log_debug "Adding rule ${YA_SLB_TUN_ROUTE_TABLE_IPV6} for ${_tun_ip}"
            ip -6 rule add from ${_tun_ip} lookup ${YA_SLB_TUN_ROUTE_TABLE_IPV6}
        fi
    done < ${_conf}

    if ip link show ${_dummy_iface} | grep -q 'DOWN' ; then
        ya_log_debug "Upping ${_dummy_iface} iface"
        ip link set dev ${_dummy_iface} up
    fi

    return 0
}

#
# ya_slb_tun_del_tun_ip slb_tunnel.conf [dummy_interface]
#    Remove SLB tunnel IP.
#
ya_slb_tun_del_tun_ip()
{
    local _conf _tun_ip _dummy_iface

    if [ ${#} -lt 1 -o -z "${1}" ] ; then
        ya_log_debug "ya_slb_tun_del_tun_ip: no configuration file specified"
        return 1
    fi

    _conf="${1}"
    if [ ! -e "${_conf}" ] ; then
        return 1
    fi

    _dummy_iface=${2:-$YA_SLB_TUN_DUMMY}
    while read _tun_ip ; do
        if ya_is_ipv4 $_tun_ip ; then
            if ! ya_check_enabled 'IF_YA_SLB_TUN' ; then
                continue
            fi

            ya_log_debug "Removing tunnel IP ${_tun_ip} from ${_dummy_iface}"
            ip addr del ${_tun_ip}/32 dev ${_dummy_iface}
        fi

        if ya_is_ipv6 $_tun_ip ; then
            if ! ya_check_enabled 'IF_YA_SLB6_TUN' ; then
                continue
            fi

            ya_log_debug "Removing rule ${YA_SLB_TUN_ROUTE_TABLE_IPV6} from ${_tun_ip}"
            ip -6 rule del from ${_tun_ip} lookup ${YA_SLB_TUN_ROUTE_TABLE_IPV6}
            ya_log_debug "Removing tunnel IP ${_tun_ip} from ${_dummy_iface}"
            ip -6 addr del ${_tun_ip}/128 dev ${_dummy_iface}
        fi
    done < $_conf

    return 0
}

#
# ya_slb_tun_up_dummy [dummy-iface]
#   Up dummy interface.
#
ya_slb_tun_up_dummy()
{
    local _dummy_iface=${1:-$YA_SLB_TUN_DUMMY}
    if ! ip addr show | grep -wq ${_dummy_iface} ; then
        if ! lsmod | grep -q dummy ; then
            modprobe dummy
        fi

        ya_log_debug "Creating ${_dummy_iface} iface"
        ip link add dev ${_dummy_iface} type dummy
        ip link set dev ${_dummy_iface} mtu ${IF_YA_SLB_TUN_MTU_IFACE} up
    fi
}

#
# ya_slb_tun_down_dummy [dummy-iface]
#   Down dummy interface.
#
ya_slb_tun_down_dummy()
{
    local _dummy_iface=${1:-$YA_SLB_TUN_DUMMY}
    if ip addr show | grep -wq ${_dummy_iface} ; then
        ya_log_debug "Removing ${_dummy_iface}"
        ip link set dev ${_dummy_iface} down
        ip link del dev ${_dummy_iface}
    fi
}

#
# ya_slb_tun_up_ipip <interface>
#   Up IPv4-IPv4 tunnel.
#
ya_slb_tun_up_ipip()
{
    local _iface _default_gw _table _mtu _mss _tun_iface

    if [ ${#} -ne 1 -o -z "$1" ] ; then
        ya_log_debug "ya_slb_tun_up_ipip: interface not specified"
        return 1
    fi

    _iface=${1}
    _tun_iface="tunl0"
    _default_gw=$(ip route | awk '/default/ && !/dev tun/ {print $3; exit}')
    _table=${YA_SLB_TUN_ROUTE_TABLE_IPV4}
    _mtu=${IF_YA_SLB_TUN_MTU}
    _mss=$((${_mtu} - 40))

    # if no ip rules by table specified, skipping
    if ! ip rule list | grep -wq "lookup ${_table}" ; then
        ya_log_debug "No rule for table ${_table} specified. Skipping up tunnel ${_tun_iface}"
        return 0
    fi

    if ! ip link show | grep -wq ${_tun_iface} ; then
        ya_log_debug "Creating tunnel dev ${_tun_iface}"
        ip tunnel add ${_tun_iface} mode ipip 2> /dev/null
    fi
    ya_log_debug "Upping dev ${_tun_iface}"
    ip link set dev ${_tun_iface} mtu ${IF_YA_SLB_TUN_MTU_IFACE} up

    if ! ip route show table ${_table} | grep -q 'default' ; then
        ya_log_debug "Adding default route for table ${_table}"
        ip route add default via ${_default_gw} dev ${_iface} \
            mtu lock ${_mtu} advmss ${_mss} table ${_table}
    fi

    return 0
}

#
# ya_slb_tun_up_ipip6 <interface>
#   Up IPv4-IPv6 tunnel.
#
ya_slb_tun_up_ipip6()
{
    local _iface _tun_iface _remote_gw _local_ip _mtu _mss _table

    if [ ${#} -ne 1 -o -z "$1" ] ; then
        ya_log_debug "ya_slb_tun_up_ipip: interface not specified"
        return 1
    fi

    _iface=${1}
    _tun_iface="tun0"
    _remote_gw="2a02:6b8:0:3400::aaaa"
    _default_gw=$(ip -6 route | awk '/default/ {print $3; exit}')
    _local_ip=$(ya_get_ip6addr ${_iface})
    _table=${YA_SLB_TUN_ROUTE_TABLE_IPV4}
    _mtu=${IF_YA_SLB_TUN_MTU}
    _mss=$((${_mtu} - 40))

    # if no ip rules by table specified, skipping
    if ! ip rule list | grep -q "lookup ${_table}" ; then
        ya_log_debug "No rule for table ${_table} specified. Skipping up tunnel ${_tun_iface}"
        return 0
    fi

    if ! ip link show | grep -wq ${_tun_iface} ; then
        ya_log_debug "Creating tunnel dev ${_tun_iface}"
        ip -6 tunnel add ${_tun_iface} mode ipip6 remote ${_remote_gw} local ${_local_ip}
    fi
    ya_log_debug "Upping dev ${_tun_iface}"
    ip link set dev ${_tun_iface} mtu ${IF_YA_SLB_TUN_MTU_IFACE} up
    ya_log_debug "Upping dev ip6tnl0"
    ip link set dev ip6tnl0 mtu ${IF_YA_SLB_TUN_MTU_IFACE} up
    ip -6 route replace ${_remote_gw} via ${_default_gw} src ${_local_ip} dev ${_iface} mtu ${IF_YA_SLB_TUN_MTU_IP4IP6}

    if ! ip route show table ${_table} | grep -q 'default' ; then
        ya_log_debug "Adding default route for table ${_table}"
        ip route add default dev ${_tun_iface} mtu lock ${_mtu} advmss ${_mss} table ${_table}
    fi

    return 0
}

#
# ya_slb_tun_up_ip6ip6 <interface>
#   Up IPv6-IPv6 tunnel.
#
ya_slb_tun_up_ip6ip6()
{
    local _iface _tun_iface _table _mtu _mss _default_gw

    if [ ${#} -ne 1 -o -z "$1" ] ; then
        ya_log_debug "ya_slb_tun_up_ip6ip6: inteface not speicified"
        return 1
    fi

    _iface=${1}
    _tun_iface="ip6tnl0"
    _table=${YA_SLB_TUN_ROUTE_TABLE_IPV6}
    _mtu=${IF_YA_SLB_TUN_MTU}
    _mss=$((${_mtu} - 60))
    _default_gw=$(ip -6 route | awk '/default/ {print $3; exit}')

    # if no ip rules by table specified, skipping
    if ! ip -6 rule list | grep -q "lookup ${_table}" ; then
        ya_log_debug "No rule for table ${_table} specified. Skipping up tunnel ${_tun_iface}"
        return 0
    fi

    if ! ip link show | grep -wq ${_tun_iface} ; then
        if ! lsmod | grep -q ip6_tunnel ; then
            modprobe ip6_tunnel
        fi
        ya_log_debug "Creating tunnel dev ${_tun_iface}"
        ip -6 tunnel add ${_tun_iface} mode ip6ip6
    fi
    ya_log_debug "Upping dev ${_tun_iface}"
    ip link set dev ${_tun_iface} mtu ${IF_YA_SLB_TUN_MTU_IFACE} up
    ip -6 tun change ${_tun_iface} mode any

    if ! ip -6 route show table ${_table} | grep -q 'default' ; then
        ya_log_debug "Adding default route for table ${_table}"
        ip -6 route add default via ${_default_gw} dev ${_iface} \
            mtu lock ${_mtu} advmss ${_mss} table ${_table}
    fi

    if ya_check_enabled 'IF_YA_SLB6_TUN_JUMBO' ; then
        if ! ip -6 route show table ${_table} | grep -q "${YA_SLB_TUN_JUMBO_IPV6_NETWORKS}" ; then
            ya_log_debug "Adding route to IPv6 jumbo frames for table  ${_table}"
            ip -6 route add ${YA_SLB_TUN_JUMBO_IPV6_NETWORKS} via ${_default_gw} \
                dev ${_iface} mtu lock ${YA_SLB_TUN_JUMBO_MTU} \
                advmss $((${YA_SLB_TUN_JUMBO_MTU} - 60)) table ${_table}
        fi
    fi

    return 0
}

#
# ya_slb_tun_down <mode>
#   Tear down tunnel according to <mode>.
#   Mode can be 'ipip', 'ipip6' or 'ip6ip6'
#
ya_slb_tun_down()
{
    local _mode _ipv _table _tun_ifaces

    _mode=${1}

    case ${_mode} in
    ipip)
        _ipv='-4'
        _table=${YA_SLB_TUN_ROUTE_TABLE_IPV4}
        _tun_ifaces=('tunl0')
        ;;
    ipip6)
        _ipv='-4'
        _table=${YA_SLB_TUN_ROUTE_TABLE_IPV4}
        _tun_ifaces=('tun0' 'ip6tnl0')
        ;;
    ip6ip6)
        _ipv='-6'
        _table=${YA_SLB_TUN_ROUTE_TABLE_IPV6}
        _tun_iface=('ip6tnl0')
        ;;
    *)
        ya_log_debug "ya_slb_tun_down: unknown mode '$_mode'"
        return 1
    esac

    ya_log_debug "Flushing table ${_table}"
    ip "${_ipv}" route flush table "${_table}"

    local _iface
    for _iface in "${_tun_ifaces[@]}" ; do
        if ip link show | grep -wq "${_iface}" ; then
            ya_log_debug "Setting down tunnel dev ${_iface}"
            ip link set dev "${_iface}" down
        fi
    done

    return 0
}

#
# ya_slb_tun_up_tunnels <physic_iface >
#   Up tunnel interfaces.
#
ya_slb_tun_up_tunnels()
{
    local _iface

    if [ ${#} -ne 1 -o -z "$1" ] ; then
        log_debug "ya_slb_tun_up_tunnels: inteface not speicified"
        return 1
    fi

    _iface=$1

    if ya_check_enabled 'IF_YA_SLB_TUN' ; then
        local _ipgw=$(ip route | awk '/default/ && !/dev tun/ {print $3; exit}')
        if [ ! -z "$_ipgw" ] ; then
            ya_slb_tun_up_ipip ${_iface}
        else
            ya_slb_tun_up_ipip6 ${_iface}
        fi

        # disable return path filtering because asymmetrical trafic
        sysctl -e -q -w net.ipv4.conf.all.rp_filter=0
        sysctl -e -q -w net.ipv4.conf.default.rp_filter=0
        for i in /proc/sys/net/ipv4/conf/*/rp_filter ; do
            echo 0 > $i ;
        done
        sysctl -e -q -w net.ipv4.conf.all.accept_redirects=0
        sysctl -e -q -w net.ipv4.conf.all.send_redirects=0
    fi

    if ya_check_enabled 'IF_YA_SLB6_TUN' ; then
        ya_slb_tun_up_ip6ip6 ${_iface}
    fi
}

#
# ya_slb_tun_down_tunnels
#   Down tunnel interfaces.
#
ya_slb_tun_down_tunnels()
{
    if ya_check_enabled 'IF_YA_SLB6_TUN' ; then
        ya_slb_tun_down ip6ip6
    fi

    if ya_check_enabled 'IF_YA_SLB_TUN' ; then
        local _ipgw=$(ip route | awk '/default/ && !/dev tun/ {print $3; exit}')
        if [ ! -z "$_ipgw" ] ; then
            ya_slb_tun_down ipip
        else
            ya_slb_tun_down ipip6
        fi
    fi
}

#
# ya_slb_tun_run_start [<interface>]
#   Start SLB tunnel.
#
ya_slb_tun_run_start()
{
    local _iface _state _dummy_iface _config

    if [ ! -z "$(ya_slb_tun_is_configured)" ] ; then
        ya_log_debug "SLB tunnel already configured"
        return 0
    fi

    _iface=$(ya_get_active_interface ${1:-${IFACE:-''}})
    if [ -z "${_iface}" ] ; then
        ya_log_debug "No active interface found or given interface not supported"
        return 1
    fi

    _config=${IF_YA_SLB_TUN_CONF_COMMON}
    # get SLB configuration
    if ! ya_slb_tun_get_config ${_iface} ${_config} ; then
        ya_log_debug "No configuration found or fetching failed for '${_iface}'"
        return 1
    fi

    if [ ! -s ${_config} ]; then
        ya_log "Empty SLB config for ${_iface}"
        return 0
    fi

    _dummy_iface=${YA_SLB_TUN_DUMMY}
    ya_slb_tun_up_dummy ${_dummy_iface}
    if ! ya_slb_tun_add_tun_ip ${_config} ${_dummy_iface} ; then
        ya_log_debug "Error when add tunnel IP to dummy iface"
        return 1
    fi
    ya_slb_tun_up_tunnels ${_iface}

    ya_slb_tun_mark_configured ${_iface}

    return 0
}

#
# ya_slb_tun_run_stop [<interface>]
#   Stop SLB tunnel.
#
ya_slb_tun_run_stop()
{
    local _iface _state _config

    _state=$(ya_slb_tun_is_configured)
    _iface=${1:-${IFACE:-${_state:-''}}}
    _config=${IF_YA_SLB_TUN_CONF_COMMON}
    if [ -z "${_state}" ] ; then
        ya_log_debug "ya-slb-tun not configured. Nothing to stop"
        return 0
    fi

    if [ "${_state}" != "${_iface}" ] ; then
        ya_log_debug "Specified interface '${_iface}', but configured '${_state}'. Stop skipping"
        return 0
    fi

    if [ ! -e ${_config} ] ; then
        if ! ya_slb_tun_get_config ${_iface} ${_config} ; then
            ya_log_debug "No configuration found or fetching failed"
            return 0
        fi
    fi

    ya_slb_tun_down_tunnels
    ya_slb_tun_del_tun_ip ${_config}
    ya_slb_tun_down_dummy

    ya_slb_tun_unmark_configured

    return 0
}

#
# ya_slb_tun_run_restart [<interface>]
#   Restart SLB tunnel.
#
ya_slb_tun_run_restart()
{
    local _iface

    _iface=${1:-''}
    ya_slb_tun_run_stop ${_iface}
    sleep 2;
    ya_slb_tun_run_start ${_iface}

    return 0
}

#
# ya_slb_tun_run_reload [<interface>]
#   Reload SLB tunnel.
#   This method won't be remove old IP, it's simplify only add a new IPs.
#
ya_slb_tun_run_reload()
{
    local _iface _state

    _state=$(ya_slb_tun_is_configured)
    _iface=${1:-${IFACE:-''}}
    if [ ! -z "${_state}" ] ; then
        if [ ! -z "${_iface}" ] ; then
            if [ "${_iface}" != "${_state}" ] ; then
                "Tunnels using '${_state}', but you want reload '${_iface}'. Use 'restart'"
                return 2
            fi
        fi

        _iface=${_state}
        ya_slb_tun_unmark_configured
    fi

    ya_slb_tun_run_start ${_iface}

    return 0
}

#
# ya_slb_tun_run <action> [<interface>]
#   Run a script.
#
ya_slb_tun_run()
{
    local _action=${1:-${MODE:-"none"}}
    local _iface=${2:-''}

    ya_slb_tun_init_vars
    mkdir -p ${YA_SLB_TUN_RUNDIR} || exit 0

    if ! (ya_check_enabled 'IF_YA_SLB_TUN' || ya_check_enabled 'IF_YA_SLB6_TUN') ; then
        ya_log_debug "$(basename $0) is off."
        exit 0
    fi

    case "${_action}" in
    start)
        ya_log_debug "Start SLB tunnels"
        ya_slb_tun_run_start ${_iface}
        exit 0
        ;;
    stop)
        ya_log_debug "Stop SLB tunnels"
        ya_slb_tun_run_stop ${_iface}
        exit 0
        ;;
    restart)
        ya_log_debug "Restart SLB tunnels"
        ya_slb_tun_run_restart ${_iface}
        exit 0
        ;;
    reload)
        ya_log_debug "Reload SLB tunnels"
        ya_slb_tun_run_reload ${_iface}
        exit $?
        ;;
    *)
        ya_slb_tun_usage
        exit 0
        ;;
    esac
}

ya_slb_tun_run "$@"

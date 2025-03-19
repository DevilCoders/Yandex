
{% set hostname = grains["nodename"] %}
{% set host_tags = grains["cluster_map"]["hosts"][hostname].get("tags", []) %}
{% set host_roles = grains["cluster_map"]["hosts"][hostname]["roles"] %}

{% if "cgw-nat" in host_roles or "cgw-dc" in host_tags%}
{% set vpp_ext_deps_version = "18.10-7" %}
{% set vpp_version = "18.10-22.1.33" %}
{% set dpdk_version = "18.02.2-vpp1"%}

{%- else %}
{# remove when CLOUD-6443 is done#}
{# FIXME: while CLOUD-6443 is not done don't forget to update remove-cgw-pkgs-old state in cloudgate/package.sls #}
{% set vpp_version = "18.04-17.15.123" %}
{% set dpdk_version = "18.02.2-vpp1" %}
{%- endif %}

yc-pinning:
  packages:
    yc-autorecovery: 0.1-4.190325
    yc-monitors: 0.1-220.190607
    libibverbs1: 43mlnx1-1.43101
    vpp: {{ vpp_version }}
    vpp-dbg: {{ vpp_version }}
    vpp-lib: {{ vpp_version }}
    vpp-api-python: {{ vpp_version }}
    vpp-plugins: {{ vpp_version }}
{%- if "cgw-nat" in host_roles or "cgw-dc" in host_tags%}
    vpp-ext-deps: {{ vpp_ext_deps_version }}
    vppsb-router-plugin: 0.2.0 #.34
    yandex-vpp-plugins: 0.1.27
    yandex-vpp-scripts: 0.2.1.123
    {%-if "bgp2vpp" not in host_tags %}
    yc-cgw-prosector: 0.1.7.34
    {% endif %}
{%- else %}
    vppsb-router-plugin: 0.1.10
    yandex-vpp-plugins: 0.1.11
    {%-if "bgp2vpp" in host_tags %}
    yandex-vpp-scripts: 0.1.33.120
    {%- else %}
    yandex-vpp-scripts: 0.1.33.114
    yc-cgw-prosector: 0.1.7.37
    {% endif %}
    vpp-dpdk-dev: {{ dpdk_version }}
    vpp-dpdk-dkms: {{ dpdk_version }}
{%- endif %}

{%-if "bgp2vpp" in host_tags %}
    gobgp: 1.33.2.38
    yc-bgp2vpp: 0.5.2-2265.190527
{% else %}
    gobgp: 1.32.3.32
{% endif %}

# see https://wiki.yandex-team.ru/cloud/devel/sdn/Network-Infra-2017/BGP-communities/
comms:
    upstream:
        import:
            border:     "13238:35130"   # border routers community
            ca:         "13238:35132"   # "cloud a" routers community, used for vpnv6 hbf
            dlbr:       "13238:35134"   # direct link border
            rkn:        "13238:35160"   # rkn filter node community
        export:
            cgw:        "13238:35150"   # cgw loopbacks community
            cgw-dc-lo:  "13238:35154"   # cgw-dc  loopbacks community
            ycnets:     "13238:35999"   # all yandex cloud routes community
    reflector:
        import:
            ip4:        "65000:6804"    # ip4 default route community
            rknall:     "65000:6600"    # all rkn routes
            rknblack:   "65000:6602"    # rkn blacklist routes
        export:
            ip4:        "65000:9002"    # ip4 aggregates route-target
            specifics:  "65000:9001"    # ip4 specifics route-target, used for lb
            noc6:       "13238:3099"    # noc ip6 community
            internal6:  "13238:35201"   # unknown purpose
            cgw-dc-agg: "65000:6501"    # cgw-dc  aggregates

hbf_rt_by_prefix:
    cut_prefix_len: 56
    prefixes:
        # SAS
        "2a02:6b8:c02:900::/56": "rt:65535:701"

        # VLA
        "2a02:6b8:c0e:500::/56": "rt:65535:702"

        # MYT
        "2a02:6b8:c03:500::/56": "rt:65535:703"



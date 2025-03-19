#define KBUILD_MODNAME "p0f_bpf"
#define __bpf__ 1 // for CLion detection, passed by clang anyway

#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/version.h>

#include <bpf_endian.h>
#include <bpf_helpers.h>

#include "p0f_bpf_types.h"

#ifdef _DEBUG
    #define bpf_debug(fmt, ...)                                        \
        ({                                                             \
            char ____fmt[] = fmt;                                      \
            bpf_trace_printk(____fmt, sizeof(____fmt), ##__VA_ARGS__); \
        })
#else
    #define bpf_debug(fmt, ...) \
        do {                    \
        } while (0)
#endif

#ifndef memset
    #define memset(dest, chr, n) __builtin_memset((dest), (chr), (n))
#endif

#ifndef memcpy
    #define memcpy(dest, src, n) __builtin_memcpy((dest), (src), (n))
#endif

struct bpf_map_def SEC("maps") jmp_table = {
    .type = BPF_MAP_TYPE_PROG_ARRAY,
    .key_size = sizeof(uint32_t),
    .value_size = sizeof(uint32_t),
    .max_entries = 8,
};

#define MAX_P0F_ITEMS 1000

struct bpf_map_def SEC("maps") p0f_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(struct in6_addr) + 2 * sizeof(uint16_t),
    .value_size = sizeof(p0f_value_t),
    .max_entries = MAX_P0F_ITEMS,
};

enum bit_fields {
    IP_ID_SET = 1,
    IP_FLOW_SET = 1 << 1,
    TCP_SEQ_ZERO = 1 << 2,
    TCP_ACK_NOTZERO_NOTSET = 1 << 3,
    TCP_ACK_ZERO_SET = 1 << 4,
    TCP_URG_NOTZERO_NOTSET = 1 << 5,
    TCP_URG_SET = 1 << 6,
    TCP_PUSH = 1 << 7
};

#define RETURN_FILTER_BYPASS return skb->len;

#define SKIP (-1)
#define DROP (-2)
#define COLLECT 0

#define INET_ECN_MASK 3

static inline __attribute__((always_inline)) void copy_ipv4(char* dst,
                                                            char* src) {
    memset(dst, 0, 12);
    memcpy(dst + 12, src, 4);
}

static inline __attribute__((always_inline)) void
copy_ipv6(char* dst, struct in6_addr* src) {
    memcpy(dst, src, 16);
}

static inline __attribute__((always_inline)) int
copy_options(struct __sk_buff* skb, p0f_value_t* p, uint32_t offset,
             uint32_t iph_size, uint8_t opts_size) {
    if (opts_size == 0) {
        return 0;
    }

    if (opts_size > sizeof(p->opts)) {
        return -1;
    }

#pragma unroll
    for (uint64_t len = 1; len <= sizeof(p->opts); ++len) {
        if (len != opts_size) {
            continue;
        }

        if (bpf_skb_load_bytes_relative(skb, ETH_HLEN + iph_size + offset, p->opts,
                                        len, BPF_HDR_START_MAC) < 0) {
            return -1;
        }

        return 0;
    }

    return -1;
}

#define KEY_IP(key, paddr) \
    { memcpy(&((key)->ip.src_addr), paddr, 4); }

#define KEY_IP6(key, paddr) \
    { memcpy(&((key)->ip6.src_addr), paddr, 16); }

#define KEY_PORTS(key, src, dst)      \
    {                                 \
        (key)->ports.src_port = src;  \
        (key)->ports.dest_port = dst; \
    }

#define KEY_LOG(...) bpf_debug(__VA_ARGS__)

static inline __attribute__((always_inline)) int
p0f_upsert(p0f_key_t* key, p0f_value_t* value) {
    bpf_debug("upserting key to map\n");
    KEY_DEBUG(key->hash);

    return bpf_map_update_elem(&p0f_map, key, value, BPF_ANY);
}

static inline __attribute__((always_inline)) int p0f_delete(p0f_key_t* key) {
    bpf_debug("removing key from map\n");
    KEY_DEBUG(key->hash);

    return bpf_map_delete_elem(&p0f_map, key);
}

static inline __attribute__((always_inline)) int
p0f_probe_tcp(struct __sk_buff* skb, uint32_t tcp_header_offset,
              uint16_t payload_size, p0f_key_t* key, p0f_value_t* value) {
    struct tcphdr tcph = {0};
    if (bpf_skb_load_bytes_relative(skb, tcp_header_offset, &tcph, sizeof(tcph),
                                    BPF_HDR_START_NET) < 0) {
        return SKIP;
    }

    bpf_debug("p0f pf: %d %d %d\n", tcph.syn, bpf_ntohs(tcph.source),
              bpf_ntohs(tcph.dest));

    if (tcph.syn && !tcph.ack) {
        // COLLECT case. This is initial packet from client
        KEY_PORTS(key, bpf_ntohs(tcph.source), bpf_ntohs(tcph.dest))

        value->tot_hdr += tcph.doff * 4;

        const uint8_t opts_size = (tcph.doff * 4) - sizeof(struct tcphdr);
        value->opts_size = opts_size;
        if (copy_options(skb, value, sizeof(tcph), tcp_header_offset, opts_size) <
            0) {
            RETURN_FILTER_BYPASS
        }

        if (tcph.seq == 0) {
            value->bit_fields ^= TCP_SEQ_ZERO;
        }

        // A good proportion of RSTs tend to have "illegal" ACK numbers, so
        // ignore these.
        if (tcph.ack_seq != 0 && tcph.ack == 0 && !tcph.rst) {
            value->bit_fields ^= TCP_ACK_NOTZERO_NOTSET;
        }

        if (tcph.ack_seq == 0 && tcph.ack == 1) {
            value->bit_fields ^= TCP_ACK_ZERO_SET;
        }

        if (tcph.urg_ptr != 0 && tcph.urg == 0) {
            value->bit_fields ^= TCP_URG_NOTZERO_NOTSET;
        }

        if (tcph.urg == 1) {
            value->bit_fields ^= TCP_URG_SET;
        }

        if (tcph.psh == 1) {
            value->bit_fields ^= TCP_PUSH;
        }

        if (tcph.ece || tcph.cwr || (tcph.res1 & 1)) {
            value->ecn = 1;
        }

        value->wsize = bpf_ntohs(tcph.window);
        value->pclass = payload_size - sizeof(struct tcphdr) - opts_size;

        value->ts = bpf_ktime_get_ns();

        return COLLECT;
    } else if (tcph.rst || tcph.fin) {
        KEY_PORTS(key, tcph.source, tcph.dest);
        return DROP;
    } else {
        bpf_debug("packet bypassed: DEST_PORT %d SEQ %d ACK %d\n", tcph.dest,
                  tcph.seq, tcph.ack_seq);
    }

    return SKIP;
}

static inline __attribute__((always_inline)) int
p0f_probe_ipv4(struct __sk_buff* skb, p0f_key_t* key, p0f_value_t* value) {
    struct iphdr iph = {0};
    uint16_t iph_size;
    uint16_t payload_size;
    uint64_t ip_proto;

    if (bpf_skb_load_bytes_relative(skb, 0, &iph, sizeof(iph),
                                    BPF_HDR_START_NET) < 0) {
        return SKIP;
    }

    iph_size = iph.ihl * 4;
    ip_proto = iph.protocol;

    if (ip_proto != IPPROTO_TCP) {
        return SKIP;
    }

    payload_size = bpf_ntohs(iph.tot_len) - iph_size;

    switch (p0f_probe_tcp(skb, iph_size, payload_size, key, value)) {
        case DROP: // when we captured RST or FIN - it's hard to tell direction of
                   // the packet, so we generate two keys and drop them
        {
            p0f_key_t inbound = *key;
            p0f_key_t outbound = {.hash = {0}};

            outbound.ports.src_port = key->ports.dest_port;
            outbound.ports.dest_port = key->ports.src_port;

            KEY_IP((&inbound), &iph.saddr);
            KEY_IP((&outbound), &iph.daddr);

            p0f_delete(&inbound);
            p0f_delete(&outbound);
        }
        case SKIP:
            return SKIP;
    }

    value->version = 4;
    value->ittl = iph.ttl;
    value->olen = iph_size - sizeof(struct iphdr);
    value->ecn = (iph.tos & INET_ECN_MASK) ? 1 : 0;
    if (iph.id != 0) {
        value->bit_fields ^= IP_ID_SET;
    }
    value->flags = bpf_ntohs(iph.frag_off);
    value->tot_hdr += iph_size;

    KEY_IP(key, &iph.saddr);

    return COLLECT;
}

static inline __attribute__((always_inline)) int
p0f_probe_ipv6(struct __sk_buff* skb, p0f_key_t* key, p0f_value_t* value) {
    struct ipv6hdr iph = {0};
    uint16_t iph_size;
    uint16_t payload_size;
    uint64_t ip_proto;

    if (bpf_skb_load_bytes_relative(skb, 0, &iph, sizeof(iph),
                                    BPF_HDR_START_NET) < 0) {
        return SKIP;
    }

    iph_size = 40;

    ip_proto = iph.nexthdr;

    struct ipv6_opt_hdr opt_hdr = {0};

#define PARSE_IPV6_EXT()                                                              \
    switch (ip_proto) {                                                               \
        case IPPROTO_HOPOPTS:                                                         \
        case IPPROTO_ROUTING:                                                         \
        case IPPROTO_FRAGMENT:                                                        \
        case IPPROTO_ICMPV6:                                                          \
        case IPPROTO_DSTOPTS:                                                         \
        case IPPROTO_MH: {                                                            \
            if (bpf_skb_load_bytes_relative(skb, iph_size, &opt_hdr, sizeof(opt_hdr), \
                                            BPF_HDR_START_NET) < 0) {                 \
                return SKIP;                                                          \
            }                                                                         \
            ip_proto = opt_hdr.nexthdr;                                               \
            iph_size += opt_hdr.hdrlen * 8 + 8;                                       \
        }                                                                             \
        default:                                                                      \
            break;                                                                    \
    }

    // eBPF doesn't have loops, so we try to parse each well-known ext-header
    // inline (up to 6 total)
    PARSE_IPV6_EXT()
    PARSE_IPV6_EXT()
    PARSE_IPV6_EXT()
    PARSE_IPV6_EXT()
    PARSE_IPV6_EXT()
    PARSE_IPV6_EXT()

    if (ip_proto != IPPROTO_TCP)
        return SKIP;

    payload_size = bpf_ntohs(iph.payload_len);

    switch (p0f_probe_tcp(skb, iph_size, payload_size, key, value)) {
        case DROP: // when we captured RST or FIN - it's hard to tell direction of
                   // the packet, so we generate two keys and drop them
        {
            p0f_key_t inbound = *key;
            p0f_key_t outbound = {.hash = {0}};

            outbound.ports.src_port = key->ports.dest_port;
            outbound.ports.dest_port = key->ports.src_port;

            KEY_IP6((&inbound), &iph.saddr)
            KEY_IP6((&outbound), &iph.daddr)

            p0f_delete(&inbound);
            p0f_delete(&outbound);
        }
        case SKIP:
            return SKIP;
    }

    value->version = 6;
    value->ittl = iph.hop_limit;
    value->olen = iph_size - sizeof(struct ipv6hdr);
    value->ecn = ((iph.flow_lbl[0] >> 4) & INET_ECN_MASK) ? 1 : 0;
    if ((iph.flow_lbl[0] & 0x0F) != 0 || iph.flow_lbl[1] != 0 ||
        iph.flow_lbl[2] != 0) {
        value->bit_fields ^= IP_FLOW_SET;
    }
    value->tot_hdr += iph_size;

    KEY_IP6(key, &iph.saddr);

    return COLLECT;
}

SEC("socket.p0f_probe")
int p0f_probe(struct __sk_buff* skb) {
    p0f_key_t key = {.hash = {0}};
    p0f_value_t value = {0};
    uint16_t mac_proto = bpf_ntohs(skb->protocol);

    switch (mac_proto) {
        case ETH_P_IP:
            if (p0f_probe_ipv4(skb, &key, &value) == COLLECT) {
                p0f_upsert(&key, &value);
            }
            break;
        case ETH_P_IPV6:
            if (p0f_probe_ipv6(skb, &key, &value) == COLLECT) {
                p0f_upsert(&key, &value);
            }
            break;
        default:
            bpf_debug("p0f pf: unexpected protocol: %d\n", skb->protocol);
    }

    RETURN_FILTER_BYPASS
}

char _license[] SEC("license") = "GPL";

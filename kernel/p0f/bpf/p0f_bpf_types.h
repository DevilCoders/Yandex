#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __bpf__
    #include <linux/in.h>
    #include <linux/in6.h>

    typedef __u8 uint8_t;
    typedef __u16 uint16_t;
    typedef __u32 uint32_t;
    typedef __u64 uint64_t;
    typedef uint16_t in_port_t;
#else
    #include <netinet/in.h>
#endif

    typedef union p0f_key_u {
        struct __attribute__((__packed__)) ip6_s {
            struct in6_addr src_addr;
            in_port_t src_port;
            in_port_t dest_port;
        } ip6;
        struct __attribute__((__packed__)) ip4_s {
            char _padding[12];
            struct in_addr src_addr;
            in_port_t src_port;
            in_port_t dest_port;
        } ip;
        struct __attribute__((__packed__)) ports_s {
            char _padding[16];
            in_port_t src_port;
            in_port_t dest_port;
        } ports;
        uint8_t hash[20];
    } p0f_key_t;

    typedef struct p0f_value_s {
        uint8_t version;
        uint8_t ittl;
        uint8_t olen;
        uint8_t pclass;
        uint8_t ecn;
        uint16_t flags;
        uint16_t wsize;
        uint16_t tot_hdr;
        uint8_t bit_fields;
        char opts[40];
        uint8_t opts_size;
        uint64_t ts;
    } p0f_value_t;

#undef KEY_LOG

#define KEY_DEBUG(key_hash)                           \
    KEY_LOG("p0f_key.hash[0] = %d\n", key_hash[0]);   \
    KEY_LOG("p0f_key.hash[1] = %d\n", key_hash[1]);   \
    KEY_LOG("p0f_key.hash[2] = %d\n", key_hash[2]);   \
    KEY_LOG("p0f_key.hash[3] = %d\n", key_hash[3]);   \
    KEY_LOG("p0f_key.hash[4] = %d\n", key_hash[4]);   \
    KEY_LOG("p0f_key.hash[5] = %d\n", key_hash[5]);   \
    KEY_LOG("p0f_key.hash[6] = %d\n", key_hash[6]);   \
    KEY_LOG("p0f_key.hash[7] = %d\n", key_hash[7]);   \
    KEY_LOG("p0f_key.hash[8] = %d\n", key_hash[8]);   \
    KEY_LOG("p0f_key.hash[9] = %d\n", key_hash[9]);   \
    KEY_LOG("p0f_key.hash[10] = %d\n", key_hash[10]); \
    KEY_LOG("p0f_key.hash[11] = %d\n", key_hash[11]); \
    KEY_LOG("p0f_key.hash[12] = %d\n", key_hash[12]); \
    KEY_LOG("p0f_key.hash[13] = %d\n", key_hash[13]); \
    KEY_LOG("p0f_key.hash[14] = %d\n", key_hash[14]); \
    KEY_LOG("p0f_key.hash[15] = %d\n", key_hash[15]); \
    KEY_LOG("p0f_key.hash[16] = %d\n", key_hash[16]); \
    KEY_LOG("p0f_key.hash[17] = %d\n", key_hash[17]); \
    KEY_LOG("p0f_key.hash[18] = %d\n", key_hash[18]); \
    KEY_LOG("p0f_key.hash[19] = %d\n", key_hash[19])

#ifdef __cplusplus
}
#endif

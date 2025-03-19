// SPDX-License-Identifier: GPL-2.0
#include <contrib/restricted/libelf/include/libelf/gelf.h>
#include <contrib/restricted/libelf/include/libelf/libelf.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/netlink.h>
#include <linux/perf_event.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(_musl_)
    #undef _IOC
    #undef _IO
    #undef _IOR
    #undef _IOW
    #undef _IOWR
#endif
#include "bpf_load.h"
#include "perf-sys.h"
#include <assert.h>
#include <contrib/libs/libbpf/src/bpf.h>
#include <ctype.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#if defined(_asan_enabled_)
void __lsan_ignore_object(const void* p);
#endif

inline static void MarkAsIntentionallyLeaked(const void* ptr) {
#if defined(_asan_enabled_)
    __lsan_ignore_object(ptr);
#else
    (void)ptr;
#endif
}

#define DEBUGFS "/sys/kernel/debug/tracing/"
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

static char license[128];
static int kern_version;
static bool processed_sec[128];
char bpf_log_buf[BPF_LOG_BUF_SIZE];
int map_fd[MAX_MAPS];
int prog_fd[MAX_PROGS];
int event_fd[MAX_PROGS];
int prog_cnt;
int prog_array_fd = -1;

struct bpf_map_data map_data[MAX_MAPS];
int map_data_count = 0;

static int populate_prog_array(const char* event, int prog_fd) {
    int ind = atoi(event), err;

    err = bpf_map_update_elem(prog_array_fd, &ind, &prog_fd, BPF_ANY);
    if (err < 0) {
        fprintf(stderr, "failed to store prog_fd in prog_array\n");
        return -1;
    }
    return prog_fd;
}

static int write_kprobe_events(const char* val) {
    int fd, ret, flags;

    if (val == NULL)
        return -1;
    else if (val[0] == '\0')
        flags = O_WRONLY | O_TRUNC;
    else
        flags = O_WRONLY | O_APPEND;

    fd = open("/sys/kernel/debug/tracing/kprobe_events", flags);

    ret = write(fd, val, strlen(val));
    close(fd);

    return ret;
}

int bpf_load_program_wrapper(enum bpf_prog_type type,
                             const struct bpf_insn* insns,
                             const char* prog_name, size_t insns_cnt,
                             const char* license, __u32 kern_version,
                             char* log_buf, size_t log_buf_sz) {
    struct bpf_load_program_attr load_attr;

    memset(&load_attr, 0, sizeof(struct bpf_load_program_attr));
    load_attr.prog_type = type;
    load_attr.expected_attach_type = 0;
    load_attr.insns = insns;
    load_attr.insns_cnt = insns_cnt;
    load_attr.license = license;
    load_attr.kern_version = kern_version;

    if (prog_name == NULL || prog_name[0] == '\0') {
        load_attr.name = NULL;
    } else {
        load_attr.name = prog_name;
    }

    return bpf_load_program_xattr(&load_attr, log_buf, log_buf_sz);
}

static int load_and_attach(const char* event, struct bpf_insn* prog, int size) {
    char prog_name[BPF_OBJ_NAME_LEN];

    memset(prog_name, 0, BPF_OBJ_NAME_LEN);
    int prog_name_len = strlen(event);

    uint32_t prog_name_pos = 0;
    bool prog_name_delim = false;
    for (int i = 0; i < prog_name_len; ++i) {
        if (event[i] == '.') {
            prog_name_delim = true;
            continue;
        }

        if (event[i] == '/') {
            break;
        }

        if (prog_name_pos > BPF_OBJ_NAME_LEN - 1) {
            break;
        }

        if (prog_name_delim) {
            prog_name[prog_name_pos++] = event[i];
        }
    }

    bool is_socket = strncmp(event, "socket", 6) == 0;
    bool is_kprobe = strncmp(event, "kprobe/", 7) == 0;
    bool is_kretprobe = strncmp(event, "kretprobe/", 10) == 0;
    bool is_tracepoint = strncmp(event, "tracepoint/", 11) == 0;
    bool is_raw_tracepoint = strncmp(event, "raw_tracepoint/", 15) == 0;
    bool is_xdp = strncmp(event, "xdp", 3) == 0;
    bool is_perf_event = strncmp(event, "perf_event", 10) == 0;
    bool is_cgroup_skb = strncmp(event, "cgroup/skb", 10) == 0;
    bool is_cgroup_sk = strncmp(event, "cgroup/sock", 11) == 0;
    bool is_sockops = strncmp(event, "sockops", 7) == 0;
    bool is_sk_skb = strncmp(event, "sk_skb", 6) == 0;
    bool is_sk_msg = strncmp(event, "sk_msg", 6) == 0;
    size_t insns_cnt = size / sizeof(struct bpf_insn);
    enum bpf_prog_type prog_type;
    char buf[256];
    int fd, efd, err, id;
    struct perf_event_attr attr = {};

    attr.type = PERF_TYPE_TRACEPOINT;
    attr.sample_type = PERF_SAMPLE_RAW;
    attr.sample_period = 1;
    attr.wakeup_events = 1;

    if (is_socket) {
        prog_type = BPF_PROG_TYPE_SOCKET_FILTER;
    } else if (is_kprobe || is_kretprobe) {
        prog_type = BPF_PROG_TYPE_KPROBE;
    } else if (is_tracepoint) {
        prog_type = BPF_PROG_TYPE_TRACEPOINT;
    } else if (is_raw_tracepoint) {
        prog_type = BPF_PROG_TYPE_RAW_TRACEPOINT;
    } else if (is_xdp) {
        prog_type = BPF_PROG_TYPE_XDP;
    } else if (is_perf_event) {
        prog_type = BPF_PROG_TYPE_PERF_EVENT;
    } else if (is_cgroup_skb) {
        prog_type = BPF_PROG_TYPE_CGROUP_SKB;
    } else if (is_cgroup_sk) {
        prog_type = BPF_PROG_TYPE_CGROUP_SOCK;
    } else if (is_sockops) {
        prog_type = BPF_PROG_TYPE_SOCK_OPS;
    } else if (is_sk_skb) {
        prog_type = BPF_PROG_TYPE_SK_SKB;
    } else if (is_sk_msg) {
        prog_type = BPF_PROG_TYPE_SK_MSG;
    } else {
        fprintf(stderr, "Unknown event '%s'\n", event);
        return -1;
    }

    if (prog_cnt == MAX_PROGS) {
        fprintf(stderr, "prog limit exceeded\n");
        return -1;
    }

    fd = bpf_load_program_wrapper(prog_type, prog, prog_name, insns_cnt, license,
                                  kern_version, bpf_log_buf, BPF_LOG_BUF_SIZE);
    if (fd < 0) {
        fprintf(stderr, "bpf_load_program() err=%d\n%s", errno, bpf_log_buf);
        return -1;
    }

    prog_fd[prog_cnt++] = fd;

    if (is_xdp || is_perf_event || is_cgroup_skb || is_cgroup_sk)
        return fd;

    if (is_socket || is_sockops || is_sk_skb || is_sk_msg) {
        if (is_socket)
            event += 6;
        else
            event += 7;
        if (*event != '/')
            return fd;
        event++;
        if (!isdigit(*event)) {
            fprintf(stderr, "invalid prog number\n");
            return -1;
        }
        return populate_prog_array(event, fd);
    }

    if (is_raw_tracepoint) {
        efd = bpf_raw_tracepoint_open(event + 15, fd);
        if (efd < 0) {
            fprintf(stderr, "tracepoint %s %s\n", event + 15, strerror(errno));
            return -1;
        }
        event_fd[prog_cnt - 1] = efd;
        return fd;
    }

    if (is_kprobe || is_kretprobe) {
        bool need_normal_check = true;
        const char* event_prefix = "";

        if (is_kprobe)
            event += 7;
        else
            event += 10;

        if (*event == 0) {
            fprintf(stderr, "event name cannot be empty\n");
            return -1;
        }

        if (isdigit(*event))
            return populate_prog_array(event, fd);

#ifdef __x86_64__
        if (strncmp(event, "sys_", 4) == 0) {
            snprintf(buf, sizeof(buf), "%c:__x64_%s __x64_%s", is_kprobe ? 'p' : 'r',
                     event, event);
            err = write_kprobe_events(buf);
            if (err >= 0) {
                need_normal_check = false;
                event_prefix = "__x64_";
            }
        }
#endif
        if (need_normal_check) {
            snprintf(buf, sizeof(buf), "%c:%s %s", is_kprobe ? 'p' : 'r', event,
                     event);
            err = write_kprobe_events(buf);
            if (err < 0) {
                fprintf(stderr, "failed to create kprobe '%s' error '%s'\n", event,
                        strerror(errno));
                return -1;
            }
        }

        strcpy(buf, DEBUGFS);
        strcat(buf, "events/kprobes/");
        strcat(buf, event_prefix);
        strcat(buf, event);
        strcat(buf, "/id");
    } else if (is_tracepoint) {
        event += 11;

        if (*event == 0) {
            fprintf(stderr, "event name cannot be empty\n");
            return -1;
        }
        strcpy(buf, DEBUGFS);
        strcat(buf, "events/");
        strcat(buf, event);
        strcat(buf, "/id");
    }

    efd = open(buf, O_RDONLY, 0);
    if (efd < 0) {
        fprintf(stderr, "failed to open event %s\n", event);
        return -1;
    }

    err = read(efd, buf, sizeof(buf));
    if (err < 0 || err >= (int)sizeof(buf)) {
        fprintf(stderr, "read from '%s' failed '%s'\n", event, strerror(errno));
        return -1;
    }

    close(efd);

    buf[err] = 0;
    id = atoi(buf);
    attr.config = id;

    efd = sys_perf_event_open(&attr, -1 /*pid*/, 0 /*cpu*/, -1 /*group_fd*/, 0);
    if (efd < 0) {
        fprintf(stderr, "event %d fd %d err %s\n", id, efd, strerror(errno));
        return -1;
    }
    event_fd[prog_cnt - 1] = efd;
    err = ioctl(efd, PERF_EVENT_IOC_ENABLE, 0);
    if (err < 0) {
        fprintf(stderr, "ioctl PERF_EVENT_IOC_ENABLE failed err %s\n",
                strerror(errno));
        return -1;
    }
    err = ioctl(efd, PERF_EVENT_IOC_SET_BPF, fd);
    if (err < 0) {
        fprintf(stderr, "ioctl PERF_EVENT_IOC_SET_BPF failed err %s\n",
                strerror(errno));
        return -1;
    }

    return fd;
}

static int load_maps(struct bpf_map_data* maps, int nr_maps,
                     fixup_map_cb fixup_map, void* fixup_context) {
    int i, numa_node;

    for (i = 0; i < nr_maps; i++) {
        numa_node =
            maps[i].def.map_flags & BPF_F_NUMA_NODE ? maps[i].def.numa_node : -1;

        if (maps[i].def.type == BPF_MAP_TYPE_ARRAY_OF_MAPS ||
            maps[i].def.type == BPF_MAP_TYPE_HASH_OF_MAPS) {
            int inner_map_fd = map_fd[maps[i].def.inner_map_idx];

            map_fd[i] = bpf_create_map_in_map_node(
                maps[i].def.type, maps[i].name, maps[i].def.key_size, inner_map_fd,
                maps[i].def.max_entries, maps[i].def.map_flags, numa_node);
        } else {
            // Comment this code for compatibility with lower kernel versions
            //             map_fd[i] = bpf_create_map_node(maps[i].def.type,
            //                             maps[i].name,
            //                             maps[i].def.key_size,
            //                             maps[i].def.value_size,
            //                             maps[i].def.max_entries,
            //                             maps[i].def.map_flags,
            //                             numa_node);
            map_fd[i] = bpf_create_map(
                maps[i].def.type, maps[i].def.key_size, maps[i].def.value_size,
                maps[i].def.max_entries, maps[i].def.map_flags);
        }
        if (map_fd[i] < 0) {
            fprintf(stderr, "failed to create a map: %d %s\n", errno,
                    strerror(errno));
            return 1;
        }
        maps[i].fd = map_fd[i];

        if (maps[i].def.type == BPF_MAP_TYPE_PROG_ARRAY)
            prog_array_fd = map_fd[i];

        if (fixup_map) {
            fixup_map(&maps[i], i, fixup_context);
        }
    }
    return 0;
}

static int get_sec(Elf* elf, int i, GElf_Ehdr* ehdr, char** shname,
                   GElf_Shdr* shdr, Elf_Data** data) {
    Elf_Scn* scn;

    scn = elf_getscn(elf, i);
    if (!scn)
        return 1;

    if (gelf_getshdr(scn, shdr) != shdr)
        return 2;

    *shname = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
    if (!*shname || !shdr->sh_size)
        return 3;

    *data = elf_getdata(scn, 0);
    if (!*data || elf_getdata(scn, *data) != NULL)
        return 4;

    return 0;
}

static int parse_relo_and_apply(Elf_Data* data, Elf_Data* symbols,
                                GElf_Shdr* shdr, struct bpf_insn* insn,
                                struct bpf_map_data* maps, int nr_maps) {
    int i, nrels;

    nrels = shdr->sh_size / shdr->sh_entsize;

    for (i = 0; i < nrels; i++) {
        GElf_Sym sym;
        GElf_Rel rel;
        unsigned int insn_idx;
        bool match = false;
        int j, map_idx;
        (void)j;

        gelf_getrel(data, i, &rel);

        insn_idx = rel.r_offset / sizeof(struct bpf_insn);

        gelf_getsym(symbols, GELF_R_SYM(rel.r_info), &sym);

        if (insn[insn_idx].code != (BPF_LD | BPF_IMM | BPF_DW)) {
            fprintf(stderr, "invalid relo for insn[%d].code 0x%x\n", insn_idx,
                    insn[insn_idx].code);
            return 1;
        }
        insn[insn_idx].src_reg = BPF_PSEUDO_MAP_FD;

        /* Match FD relocation against recorded map_data[] offset */
        for (map_idx = 0; map_idx < nr_maps; map_idx++) {
            if (maps[map_idx].elf_offset == sym.st_value) {
                match = true;
                break;
            }
        }
        if (match) {
            insn[insn_idx].imm = maps[map_idx].fd;
        } else {
            fprintf(stderr, "invalid relo for insn[%d] no map_data match\n",
                    insn_idx);
            return 1;
        }
    }

    return 0;
}

static int cmp_symbols(const void* l, const void* r) {
    const GElf_Sym* lsym = (const GElf_Sym*)l;
    const GElf_Sym* rsym = (const GElf_Sym*)r;

    if (lsym->st_value < rsym->st_value)
        return -1;
    else if (lsym->st_value > rsym->st_value)
        return 1;
    else
        return 0;
}

static int load_elf_maps_section(struct bpf_map_data* maps, int maps_shndx,
                                 Elf* elf, Elf_Data* symbols, int strtabidx) {
    int map_sz_elf, map_sz_copy;
    bool validate_zero = false;
    Elf_Data* data_maps;
    int i, nr_maps;
    GElf_Sym* sym;
    Elf_Scn* scn;
    int copy_sz;
    (void)copy_sz;

    if (maps_shndx < 0)
        return -EINVAL;
    if (!symbols)
        return -EINVAL;

    /* Get data for maps section via elf index */
    scn = elf_getscn(elf, maps_shndx);
    if (scn)
        data_maps = elf_getdata(scn, NULL);
    if (!scn || !data_maps) {
        fprintf(stderr, "Failed to get Elf_Data from maps section %d\n",
                maps_shndx);
        return -EINVAL;
    }

    /* For each map get corrosponding symbol table entry */
    sym = calloc(MAX_MAPS + 1, sizeof(GElf_Sym));
    for (i = 0, nr_maps = 0; i < (int)symbols->d_size / (int)sizeof(GElf_Sym);
         i++) {
        assert(nr_maps < MAX_MAPS + 1);
        if (!gelf_getsym(symbols, i, &sym[nr_maps]))
            continue;
        if (sym[nr_maps].st_shndx != maps_shndx)
            continue;
        /* Only increment iif maps section */
        nr_maps++;
    }

    /* Align to map_fd[] order, via sort on offset in sym.st_value */
    qsort(sym, nr_maps, sizeof(GElf_Sym), cmp_symbols);

    /* Keeping compatible with ELF maps section changes
   * ------------------------------------------------
   * The program size of struct bpf_load_map_def is known by loader
   * code, but struct stored in ELF file can be different.
   *
   * Unfortunately sym[i].st_size is zero.  To calculate the
   * struct size stored in the ELF file, assume all struct have
   * the same size, and simply divide with number of map
   * symbols.
   */
    map_sz_elf = data_maps->d_size / nr_maps;
    map_sz_copy = sizeof(struct bpf_load_map_def);
    if (map_sz_elf < map_sz_copy) {
        /*
     * Backward compat, loading older ELF file with
     * smaller struct, keeping remaining bytes zero.
     */
        map_sz_copy = map_sz_elf;
    } else if (map_sz_elf > map_sz_copy) {
        /*
     * Forward compat, loading newer ELF file with larger
     * struct with unknown features. Assume zero means
     * feature not used.  Thus, validate rest of struct
     * data is zero.
     */
        validate_zero = true;
    }

    /* Memcpy relevant part of ELF maps data to loader maps */
    for (i = 0; i < nr_maps; i++) {
        struct bpf_load_map_def* def;
        unsigned char *addr, *end;
        const char* map_name;
        size_t offset;

        map_name = elf_strptr(elf, strtabidx, sym[i].st_name);
        maps[i].name = strdup(map_name);
        MarkAsIntentionallyLeaked(maps[i].name);
        if (!maps[i].name) {
            fprintf(stderr, "strdup(%s): %s(%d)\n", map_name, strerror(errno), errno);
            free(sym);
            return -errno;
        }

        /* Symbol value is offset into ELF maps section data area */
        offset = sym[i].st_value;
        def = (struct bpf_load_map_def*)(data_maps->d_buf + offset);
        maps[i].elf_offset = offset;
        memset(&maps[i].def, 0, sizeof(struct bpf_load_map_def));
        memcpy(&maps[i].def, def, map_sz_copy);

        /* Verify no newer features were requested */
        if (validate_zero) {
            addr = (unsigned char*)def + map_sz_copy;
            end = (unsigned char*)def + map_sz_elf;
            for (; addr < end; addr++) {
                if (*addr != 0) {
                    free(sym);
                    return -EFBIG;
                }
            }
        }
    }

    free(sym);
    return nr_maps;
}

static int do_load_bpf(Elf* elf, fixup_map_cb fixup_map, void* fixup_context) {
    int i, ret, maps_shndx = -1, strtabidx = -1;
    GElf_Ehdr ehdr;
    GElf_Shdr shdr, shdr_prog;
    Elf_Data *data, *data_prog, *data_maps = NULL, *symbols = NULL;
    char *shname, *shname_prog;
    int nr_maps = 0;

    if (!elf) {
        fprintf(stderr, "*elf is null\n");
        return -1;
    }

    if (gelf_getehdr(elf, &ehdr) != &ehdr) {
        fprintf(stderr, "failed to read ELF header: %s\n", elf_errmsg(elf_errno()));
        return -1;
    }

    /* clear all kprobes */
    i = write_kprobe_events("");

    /* scan over all elf sections to get license and map info */
    for (i = 1; i < ehdr.e_shnum; i++) {
        if (get_sec(elf, i, &ehdr, &shname, &shdr, &data))
            continue;

        if (strcmp(shname, "license") == 0) {
            processed_sec[i] = true;
            memcpy(license, data->d_buf, data->d_size);
        } else if (strcmp(shname, "version") == 0) {
            processed_sec[i] = true;
            if (data->d_size != sizeof(int)) {
                fprintf(stderr, "invalid size of version section %zd\n", data->d_size);
                return -1;
            }
            memcpy(&kern_version, data->d_buf, sizeof(int));
        } else if (strcmp(shname, "maps") == 0) {
            int j;

            maps_shndx = i;
            data_maps = data;
            for (j = 0; j < MAX_MAPS; j++)
                map_data[j].fd = -1;
        } else if (shdr.sh_type == SHT_SYMTAB) {
            strtabidx = shdr.sh_link;
            symbols = data;
        }
    }

    ret = 1;

    if (!symbols) {
        fprintf(stderr, "missing SHT_SYMTAB section\n");
        goto done;
    }

    if (data_maps) {
        nr_maps =
            load_elf_maps_section(map_data, maps_shndx, elf, symbols, strtabidx);
        if (nr_maps < 0) {
            fprintf(stderr, "Error: Failed loading ELF maps (errno:%d):%s\n", nr_maps,
                    strerror(-nr_maps));
            goto done;
        }
        if (load_maps(map_data, nr_maps, fixup_map, fixup_context)) {
            goto done;
        }
        map_data_count = nr_maps;

        processed_sec[maps_shndx] = true;
    }

    /* process all relo sections, and rewrite bpf insns for maps */
    for (i = 1; i < ehdr.e_shnum; i++) {
        if (processed_sec[i])
            continue;

        if (get_sec(elf, i, &ehdr, &shname, &shdr, &data))
            continue;

        if (shdr.sh_type == SHT_REL) {
            struct bpf_insn* insns;

            /* locate prog sec that need map fixup (relocations) */
            if (get_sec(elf, shdr.sh_info, &ehdr, &shname_prog, &shdr_prog,
                        &data_prog))
                continue;

            if (shdr_prog.sh_type != SHT_PROGBITS ||
                !(shdr_prog.sh_flags & SHF_EXECINSTR))
                continue;

            insns = (struct bpf_insn*)data_prog->d_buf;
            processed_sec[i] = true; /* relo section */

            if (parse_relo_and_apply(data, symbols, &shdr, insns, map_data, nr_maps))
                continue;
        }
    }

    /* load programs */
    for (i = 1; i < ehdr.e_shnum; i++) {
        if (processed_sec[i])
            continue;

        if (get_sec(elf, i, &ehdr, &shname, &shdr, &data))
            continue;

        if (memcmp(shname, "kprobe/", 7) == 0 ||
            memcmp(shname, "kretprobe/", 10) == 0 ||
            memcmp(shname, "tracepoint/", 11) == 0 ||
            memcmp(shname, "raw_tracepoint/", 15) == 0 ||
            memcmp(shname, "xdp", 3) == 0 ||
            memcmp(shname, "perf_event", 10) == 0 ||
            memcmp(shname, "socket", 6) == 0 || memcmp(shname, "cgroup/", 7) == 0 ||
            memcmp(shname, "sockops", 7) == 0 || memcmp(shname, "sk_skb", 6) == 0 ||
            memcmp(shname, "sk_msg", 6) == 0) {
            ret = load_and_attach(shname, data->d_buf, data->d_size);
            if (ret < 0) {
                fprintf(stderr, "failed to load bpf code\n");
                goto done;
            }
        }
    }

done:
    elf_end(elf); /// Added it so there is no memory leak
    return ret;
}

static int do_load_bpf_file(const char* path, fixup_map_cb fixup_map,
                            void* fixup_context) {
    int fd;
    Elf* elf;

    /* reset global variables */
    kern_version = 0;
    memset(license, 0, sizeof(license));
    memset(processed_sec, 0, sizeof(processed_sec));

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "wrong ELF version: %s\n", elf_errmsg(elf_errno()));
        return -1;
    }

    fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        fprintf(stderr, "can't open file: %s\n", strerror(errno));
        return -1;
    }

    elf = elf_begin(fd, ELF_C_READ, NULL);
    int res = do_load_bpf(elf, fixup_map, fixup_context);
    close(fd);
    return res;
}

static int do_load_bpf_image(char* image, const size_t image_len,
                             fixup_map_cb fixup_map, void* fixup_context) {
    Elf* elf;

    /* reset global variables */
    kern_version = 0;
    memset(license, 0, sizeof(license));
    memset(processed_sec, 0, sizeof(processed_sec));

    if (elf_version(EV_CURRENT) == EV_NONE)
        return 1;

    elf = elf_memory(image, image_len);
    return do_load_bpf(elf, fixup_map, fixup_context);
}

int load_bpf_file(char* path) {
    return do_load_bpf_file(path, NULL, NULL);
}

int load_bpf_file_fixup_map(const char* path, fixup_map_cb fixup_map,
                            void* fixup_context) {
    return do_load_bpf_file(path, fixup_map, fixup_context);
}

int load_bpf_image(char* image, const size_t image_len) {
    return do_load_bpf_image(image, image_len, NULL, NULL);
}

int load_bpf_image_fixup_map(char* image, const size_t image_len,
                             fixup_map_cb fixup_map, void* fixup_context) {
    return do_load_bpf_image(image, image_len, fixup_map, fixup_context);
}

void read_trace_pipe(void) {
    int trace_fd;

    trace_fd = open(DEBUGFS "trace_pipe", O_RDONLY, 0);
    if (trace_fd < 0)
        return;

    while (1) {
        static char buf[4096];
        ssize_t sz;

        sz = read(trace_fd, buf, sizeof(buf) - 1);
        if (sz > 0) {
            buf[sz] = 0;
            puts(buf);
        }
    }
}

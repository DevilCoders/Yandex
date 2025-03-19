BUILD_ONLY_IF(LINUX)

LIBRARY()

SET(BPF_LLVM_OPTS
    -D__KERNEL__
    -D__BPF_TRACING__
    -D__TARGET_ARCH_x86
    -D__x86_64__
    -g -O2
)

PEERDIR(
    contrib/libs/libbpf
)

DEFAULT(_DEBUG 0)

IF (_DEBUG)
    SET_APPEND(BPF_LLVM_OPTS -D_DEBUG)
ENDIF()

BPF_STATIC(bpf_program.c p0f_bpf ${BPF_LLVM_OPTS})

RESOURCE(
    p0f_bpf /p0f_bpf
)

END()

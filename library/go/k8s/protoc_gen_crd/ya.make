GO_PROGRAM(protoc-gen-crd)

OWNER(g:infractl)

SRCS(main.go)

PEERDIR(library/go/k8s/protoc_gen_crd/proto)

END()

RECURSE(proto)

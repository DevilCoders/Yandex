PEERDIR(library/go/k8s/protoc_gen_crd/proto)

GO_PROTO_PLUGIN(
    go_proto_crd
    .crd.yaml
    library/go/k8s/protoc_gen_crd
)

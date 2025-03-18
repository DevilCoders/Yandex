#!/usr/bin/env bash
proto_tmp=$(mktemp --directory)/protobuf
protoc_path=$ARCADIA_ROOT/contrib/tools/protoc

git clone --depth 1 --filter=blob:none --sparse https://github.com/protocolbuffers/protobuf.git $proto_tmp
pushd $proto_tmp && git sparse-checkout set src; popd

ya m $protoc_path
$protoc_path/protoc \
    --descriptor_set_out $ARCADIA_ROOT/logfeller/configs/parsers/ci-events.desc \
    --proto_path $proto_tmp/src \
    --proto_path $ARCADIA_ROOT \
    --include_imports --include_source_info \
    -I$ARCADIA_ROOT/ci/proto/event \
    $ARCADIA_ROOT/ci/proto/event/event.proto


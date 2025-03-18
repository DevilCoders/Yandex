set -xe

ARCADIA=~/arc/arcadia

cd $ARCADIA/contrib/tools/protoc/bin
ya make

./protoc \
    --descriptor_set_out $ARCADIA/logfeller/configs/parsers/antirobot-daemon-logs.desc \
    --proto_path $ARCADIA \
    --proto_path $ARCADIA/contrib/libs/protobuf/src \
    --include_imports \
    --include_source_info \
    $ARCADIA/antirobot/idl/daemon_log.proto

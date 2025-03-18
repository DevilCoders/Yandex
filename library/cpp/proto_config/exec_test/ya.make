EXECTEST()

OWNER(
    avitella
    mvel
    g:iss
)

DEPENDS(
    library/cpp/proto_config/exec_test/tool
    library/cpp/proto_config/exec_test/tool_resource
)

DATA(arcadia/library/cpp/proto_config/exec_test/tool_resource/config.json)

RUN(
    tool
    -h
    STDERR
    usage.txt
    CANONIZE
    usage.txt
)

RUN(
    tool_resource
    -h
    STDERR
    usage_resource.txt
    CANONIZE
    usage_resource.txt
)

RUN(
    tool_resource
    STDOUT
    resource_default.txt
    CANONIZE
    resource_default.txt
)

RUN(
    tool_resource
    -c
    ${ARCADIA_ROOT}/library/cpp/proto_config/exec_test/tool_resource/config.json
    STDOUT
    resource_override.txt
    CANONIZE
    resource_override.txt
)

RUN(
    tool_resource
    -c
    ${ARCADIA_ROOT}/library/cpp/proto_config/exec_test/tool_resource/config.pb.txt
    --format
    ProtoText
    STDOUT
    resource_proto_text_config.txt
    CANONIZE
    resource_proto_text_config.txt
)

RUN(
    tool_resource
    -r
    /config/config.pb.txt
    --format
    ProtoText
    STDOUT
    resource_from_cmd_args_proto_text_config.txt
    CANONIZE
    resource_from_cmd_args_proto_text_config.txt
)

RUN(
    tool_resource
    -r
    /config/config2.pb.txt
    --format
    ProtoText
    STDOUT
    resource_from_cmd_args_proto_text_config2.txt
    CANONIZE
    resource_from_cmd_args_proto_text_config2.txt
)



END()

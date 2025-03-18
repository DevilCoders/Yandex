OWNER(
    g:geoapps_infra
)

LIBRARY()

ADDINCLSELF()

PEERDIR(
    contrib/libs/ctemplate
    contrib/libs/protobuf
    contrib/libs/protoc
    contrib/restricted/boost/libs/regex
    contrib/restricted/boost/system
    tools/idl/utils
    tools/idl/common
    tools/idl/parser
)

MAPSMOBI_COLLECT_TPL_FILES(_TEMPLATES DIR templates)

PYTHON(
    mk_templates.py ${CURDIR}/templates ${_TEMPLATES}
    IN ${_TEMPLATES}
    STDOUT tpl_cache.cpp
    OUTPUT_INCLUDES tpl/cache.h
    CWD ${ARCADIA_ROOT}/tools/idl/generator
)

SRCS(
    jni_cpp/common.cpp
    jni_cpp/struct_maker.cpp
    jni_cpp/function_maker.cpp
    jni_cpp/generation.cpp
    jni_cpp/import_maker.cpp
    jni_cpp/variant_maker.cpp
    jni_cpp/interface_maker.cpp
    jni_cpp/jni.cpp
    jni_cpp/listener_maker.cpp
    tpl/tpl.cpp
    tests/doc_maker_tests.cpp
    tests/test_helpers.cpp
    tests/protoconv/protoconv_tests.cpp
    tests/objc/low_level_tests.cpp
    java/type_name_maker.cpp
    java/serialization_addition.cpp
    java/struct_maker.cpp
    java/function_maker.cpp
    java/enum_field_maker.cpp
    java/doc_maker.cpp
    java/generation.cpp
    java/annotation_addition.cpp
    java/import_maker.cpp
    java/listener_maker.cpp
    java/variant_maker.cpp
    java/interface_maker.cpp
    obj_cpp/common.cpp
    obj_cpp/struct_maker.cpp
    obj_cpp/function_maker.cpp
    obj_cpp/generation.cpp
    obj_cpp/import_maker.cpp
    obj_cpp/variant_maker.cpp
    obj_cpp/interface_maker.cpp
    obj_cpp/listener_maker.cpp
    common/generator.cpp
    common/common.cpp
    common/type_name_maker.cpp
    common/struct_maker.cpp
    common/function_maker.cpp
    common/enum_maker.cpp
    common/pod_decider.cpp
    common/enum_field_maker.cpp
    common/doc_maker.cpp
    common/import_maker.cpp
    common/variant_maker.cpp
    common/interface_maker.cpp
    common/listener_maker.cpp
    protoconv/generator.cpp
    protoconv/common.cpp
    protoconv/error_checker.cpp
    protoconv/cpp_generator.cpp
    protoconv/header_generator.cpp
    protoconv/base_generator.cpp
    protoconv/proto_file.cpp
    cpp/bitfield_operators.cpp
    cpp/common.cpp
    cpp/type_name_maker.cpp
    cpp/weak_ref_create_platforms.cpp
    cpp/serialization_addition.cpp
    cpp/struct_maker.cpp
    cpp/function_maker.cpp
    cpp/enum_maker.cpp
    cpp/enum_field_maker.cpp
    cpp/generation.cpp
    cpp/import_maker.cpp
    cpp/guid_registration_maker.cpp
    cpp/variant_maker.cpp
    cpp/interface_maker.cpp
    cpp/listener_maker.cpp
    cpp/traits_makers.cpp
    objc/common.cpp
    objc/type_name_maker.cpp
    objc/serialization_addition.cpp
    objc/struct_maker.cpp
    objc/function_maker.cpp
    objc/enum_maker.cpp
    objc/pod_decider.cpp
    objc/enum_field_maker.cpp
    objc/doc_maker.cpp
    objc/generation.cpp
    objc/import_maker.cpp
    objc/variant_maker.cpp
    objc/interface_maker.cpp
    objc/listener_maker.cpp
)

END()

RECURSE_FOR_TESTS(tests)

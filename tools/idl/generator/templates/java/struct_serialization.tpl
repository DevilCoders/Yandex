{{#LITE}}@Override
public void serialize(Archive archive) {{{#FIELD}}
    {{FIELD_NAME}} = archive.add({{FIELD_NAME}}{{#OPTIONAL}}, {{OPTIONAL_VALUE}}{{/OPTIONAL}}{{#CUSTOM}},
        {{FIELD_CLASS}}.class{{/CUSTOM}}{{#HANDLER}},
        new {{HANDLER_TYPE}}Handler{{#GENERIC_HANDLER}}<{{GENERIC_CLASS}}>{{/GENERIC_HANDLER}}({{#CUSTOM_HANDLER}}{{FIELD_CLASS}}.class{{#CUSTOM_HANDLER_separator}},
            {{/CUSTOM_HANDLER_separator}}{{/CUSTOM_HANDLER}}){{/HANDLER}});{{/FIELD}}
}{{/LITE}}{{#BRIDGED}}@Override
public void serialize(Archive archive) {
    if (archive.isReader()) {
        ByteBuffer buffer = null;
        buffer = archive.add(buffer);
        nativeObject = loadNative(buffer);
    } else {
        ByteBuffer buffer = ByteBuffer.allocateDirect(10);
        archive.add(saveNative());
    }
}

private static native NativeObject loadNative(ByteBuffer buffer);
private native ByteBuffer saveNative();{{/BRIDGED}}
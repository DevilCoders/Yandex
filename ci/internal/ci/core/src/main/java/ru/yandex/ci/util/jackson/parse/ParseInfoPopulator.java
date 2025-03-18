package ru.yandex.ci.util.jackson.parse;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.BeanDescription;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.deser.ResolvableDeserializer;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;
import com.google.common.base.Preconditions;

class ParseInfoPopulator extends StdDeserializer<Object> implements ResolvableDeserializer {

    private final JsonDeserializer<?> defaultDeserializer;

    ParseInfoPopulator(BeanDescription beanClass, JsonDeserializer<?> defaultDeserializer) {
        super(beanClass.getBeanClass());
        this.defaultDeserializer = defaultDeserializer;
    }

    @Override
    public Object deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        Object deserialized = defaultDeserializer.deserialize(p, ctxt);
        if (deserialized instanceof HasParseInfo hasParseInfo) {
            String path = ctxt.getParser().getParsingContext().pathAsPointer().toString();
            Preconditions.checkState(hasParseInfo.getParseInfo() == null, "%s already initialized", path);
            hasParseInfo.setParseInfo(new ParseInfo(path));
            return hasParseInfo;
        } else {
            return deserialized;
        }
    }

    @Override
    public void resolve(DeserializationContext ctxt) throws JsonMappingException {
        ((ResolvableDeserializer) defaultDeserializer).resolve(ctxt);
    }
}

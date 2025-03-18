package ru.yandex.ci.util.jackson.parse;

import com.fasterxml.jackson.databind.BeanDescription;
import com.fasterxml.jackson.databind.DeserializationConfig;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.deser.BeanDeserializerModifier;

class ParseInfoProviderRegistrar extends BeanDeserializerModifier {

    @Override
    public JsonDeserializer<?> modifyDeserializer(
            DeserializationConfig config,
            BeanDescription beanDesc,
            JsonDeserializer<?> deserializer
    ) {
        var beanClass = beanDesc.getBeanClass();
        if (HasParseInfo.class.isAssignableFrom(beanClass) || beanClass.getName().endsWith("$Builder")) {
            return new ParseInfoPopulator(beanDesc, deserializer);
        }
        return deserializer;
    }
}

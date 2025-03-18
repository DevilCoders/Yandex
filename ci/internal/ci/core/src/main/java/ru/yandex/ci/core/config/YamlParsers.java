package ru.yandex.ci.core.config;

import java.io.IOException;
import java.io.InputStream;
import java.io.Reader;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.regex.Pattern;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.core.io.IOContext;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.PropertyNamingStrategies;
import com.fasterxml.jackson.databind.PropertyNamingStrategy;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.dataformat.yaml.YAMLFactory;
import com.fasterxml.jackson.dataformat.yaml.YAMLGenerator;
import com.fasterxml.jackson.dataformat.yaml.YAMLParser;
import com.fasterxml.jackson.datatype.jsr310.deser.LocalTimeDeserializer;
import com.github.fge.jsonschema.core.load.configuration.LoadingConfiguration;
import com.github.fge.jsonschema.main.JsonSchema;
import com.github.fge.jsonschema.main.JsonSchemaFactory;
import com.google.common.base.Preconditions;
import lombok.Value;
import org.springframework.beans.DirectFieldAccessor;
import org.yaml.snakeyaml.nodes.Tag;
import org.yaml.snakeyaml.resolver.Resolver;

import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.ci.util.jackson.parse.ParseInfoModule;

public class YamlParsers {
    public static final Pattern BOOL = Pattern
            .compile("^(?:yes|Yes|YES|no|No|NO|true|True|TRUE|false|False|FALSE)$");

    private YamlParsers() {
        //
    }

    public static JsonSchema buildValidationSchema(ObjectMapper mapper, String schemaPath, SchemaMapping... preloads) {
        try {
            var loadingConfigurationBuilder = LoadingConfiguration.newBuilder();
            for (var preload : preloads) {
                // we cannot store schema $id in a schema itself
                // validator tries download schema by provided uri
                // https://github.com/java-json-tools/json-schema-validator/issues/214
                loadingConfigurationBuilder.preloadSchema(
                        preload.getUri(),
                        mapper.readTree(ResourceUtils.url(preload.getResource()))
                );
            }
            return JsonSchemaFactory.newBuilder()
                    .setLoadingConfiguration(loadingConfigurationBuilder.freeze())
                    .freeze()
                    .getJsonSchema(mapper.readTree(ResourceUtils.url(schemaPath)));
        } catch (Exception ex) {
            throw new RuntimeException(ex);
        }
    }

    @Value(staticConstructor = "of")
    public static class SchemaMapping {
        String uri;
        String resource;
    }

    public static ObjectMapper buildMapper() {
        return buildMapper(PropertyNamingStrategies.KEBAB_CASE);
    }

    public static ObjectMapper buildMapper(@Nullable PropertyNamingStrategy propertyNamingStrategy) {
        YAMLFactory yamlFactory = new YAMLFactory() {
            @Override
            protected YAMLParser _createParser(InputStream in, IOContext ctxt) throws IOException {
                return fixParser(super._createParser(in, ctxt));
            }

            @Override
            protected YAMLParser _createParser(Reader r, IOContext ctxt) throws IOException {
                return fixParser(super._createParser(r, ctxt));
            }

            @Override
            protected YAMLParser _createParser(char[] data, int offset, int len, IOContext ctxt,
                                               boolean recyclable) throws IOException {
                return fixParser(super._createParser(data, offset, len, ctxt, recyclable));
            }

            @Override
            protected YAMLParser _createParser(byte[] data, int offset, int len, IOContext ctxt) throws IOException {
                return fixParser(super._createParser(data, offset, len, ctxt));
            }
        }
                .disable(YAMLGenerator.Feature.WRITE_DOC_START_MARKER)
                .enable(YAMLGenerator.Feature.MINIMIZE_QUOTES);

        var configModule = new SimpleModule()
                .addDeserializer(
                        LocalTime.class,
                        new LocalTimeDeserializer(DateTimeFormatter.ofPattern("H:mm", Locale.US))
                );

        var mapper = new ObjectMapper(yamlFactory)
                .registerModule(configModule)
                .registerModule(new ParseInfoModule())
                .setSerializationInclusion(JsonInclude.Include.NON_EMPTY);

        if (propertyNamingStrategy != null) {
            mapper.setPropertyNamingStrategy(propertyNamingStrategy);
        }

        return mapper;
    }

    private static YAMLParser fixParser(YAMLParser yamlParser) {
        Resolver resolver = getProperty(yamlParser, "_yamlResolver");
        fixResolver(resolver);
        return yamlParser;
    }

    public static Resolver fixResolver(Resolver resolver) {
        Map<Character, List<?>> tuples = getProperty(resolver, "yamlImplicitResolvers");
        for (var listIter = tuples.entrySet().iterator(); listIter.hasNext(); ) {
            var entry = listIter.next();
            var list = entry.getValue();
            for (var iter = list.iterator(); iter.hasNext(); ) {
                Tag tag = getProperty(iter.next(), "tag");
                if (Tag.BOOL.equals(tag)) {
                    iter.remove();
                }
            }
            if (list.isEmpty()) {
                listIter.remove();
            }
        }

        resolver.addImplicitResolver(Tag.BOOL, BOOL, "yYnNtTfF");
        return resolver;
    }

    @SuppressWarnings({"unchecked", "TypeParameterUnusedInFormals"})
    private static <T> T getProperty(@Nonnull Object object, @Nonnull String propertyName) {
        var value = new DirectFieldAccessor(object).getPropertyValue(propertyName);
        Preconditions.checkState(value != null, "Cannot access %s.%s",
                object.getClass(), propertyName);
        return (T) value;
    }
}

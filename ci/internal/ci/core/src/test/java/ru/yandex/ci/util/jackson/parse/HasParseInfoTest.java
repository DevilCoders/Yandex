package ru.yandex.ci.util.jackson.parse;

import java.io.IOException;
import java.time.Duration;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.NullNode;
import com.google.common.reflect.ClassPath;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.CleanupConfig;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.test.random.DurationRandomizer;
import ru.yandex.ci.test.random.JsonObjectRandomizer;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
class HasParseInfoTest {

    private static final long SEED = 1036611578L;

    private static final EnhancedRandom RANDOM = new EnhancedRandomBuilder()
            .seed(SEED)
            .objectPoolSize(10)
            // set collectionSizeRange = [0, 0) to escape StackOverflowError
            .collectionSizeRange(0, 0)
            .randomizationDepth(10)
            .randomize(Duration.class, new DurationRandomizer(SEED))
            .randomize(JsonObject.class, new JsonObjectRandomizer(SEED))
            .randomize(JsonNode.class, (Randomizer<NullNode>) NullNode::getInstance)
            // EnhancedRandom fails with this classes
            .excludeType(cls ->
                    CleanupConfig.class.isAssignableFrom(cls)
                            || JsonElement.class.isAssignableFrom(cls)
                            || JobAttemptsConfig.class.isAssignableFrom(cls)
            )
            .build();

    @Test
    void parseInfoShouldBeKeptAfterCallingLombokWithMethod() throws IOException {
        ClassPath.from(ClassLoader.getSystemClassLoader())
                .getAllClasses()
                .stream()
                .filter(clazz -> clazz.getPackageName().startsWith("ru.yandex.ci."))
                .map(ClassPath.ClassInfo::load)
                .filter(HasParseInfo.class::isAssignableFrom)
                // skip builders, cause we don't know how to serialized them
                .filter(clazz -> !clazz.getName().endsWith("Builder"))
                .filter(clazz -> !clazz.isInterface())
                .forEach(clazz -> {
                    try {
                        log.info("Processing " + clazz.getName());
                        // generate object to guarantee that all fields won't be null in order to pass @Nonnull checks
                        var json = AYamlParser.getMapper().writeValueAsString(RANDOM.nextObject(clazz));

                        log.info("Deserializing json: {}", json);
                        Object object = AYamlParser.getMapper().readValue(json, clazz);

                        var parseInfo = ((HasParseInfo) object).getParseInfo();

                        assertThat(parseInfo.getParsedPath())
                                .describedAs("Parsed path should be initialized")
                                .isNotNull();

                        // get method, generated for fields with '@With' annotation
                        var withMethod = Arrays.stream(clazz.getMethods())
                                .filter(it -> it.getName().startsWith("with"))
                                .filter(it -> it.getParameterCount() == 1)
                                .findFirst()
                                .orElse(null);

                        if (withMethod != null) {
                            log.info("Checking method: {}", withMethod);

                            var expectParameterType = withMethod.getParameterTypes()[0];
                            Object parameter = RANDOM.nextObject(expectParameterType);

                            if (parameter.getClass() == HashMap.class && expectParameterType == LinkedHashMap.class) {
                                parameter = new LinkedHashMap<>();
                            }
                            var parsedInfoOfModifiedObject = ((HasParseInfo) withMethod.invoke(object, parameter))
                                    .getParseInfo();

                            assertThat(parsedInfoOfModifiedObject)
                                    .describedAs("ParseInfo should be kept after call of '%s.%s' method".formatted(
                                            clazz.getName(), withMethod.getName()
                                    ))
                                    .isEqualTo(parseInfo);
                        }
                    } catch (Exception e) {
                        if (e instanceof RuntimeException) {
                            throw (RuntimeException) e;
                        }
                        throw new RuntimeException(e);
                    }
                });
    }

}

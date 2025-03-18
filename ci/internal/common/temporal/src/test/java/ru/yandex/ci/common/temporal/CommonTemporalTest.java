package ru.yandex.ci.common.temporal;

import java.lang.reflect.Modifier;
import java.util.stream.Stream;

import io.github.benas.randombeans.EnhancedRandomBuilder;
import io.github.benas.randombeans.api.EnhancedRandom;
import lombok.extern.slf4j.Slf4j;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.MethodSource;
import org.reflections.Reflections;

import ru.yandex.ci.common.temporal.config.TemporalConfigurationUtil;

/**
 * Этот класс надо заимплеменьтить в любом модуле, в котором есть воркеры темпорала.
 * Все остальное случится автомагичестки благодаря рефлекшену.
 */
@Slf4j
public abstract class CommonTemporalTest {
    private static final long SEED = 1734221036L;

    private static final EnhancedRandom RANDOM = new EnhancedRandomBuilder()
            .seed(SEED)
            .overrideDefaultInitialization(true)
            .build();

    /**
     * Тест проверяющий, что все классы, используемы как ID для темпорала могут корректно десериализоваться
     * используя его dataConverter
     */
    @ParameterizedTest
    @MethodSource("temporalIdClasses")
    void validateDeserialization(Class<? extends BaseTemporalWorkflow.Id> clazz) {

        var object = RANDOM.nextObject(clazz);
        var payloadOptional = TemporalConfigurationUtil.dataConverter().toPayload(object);

        Assertions.assertThat(payloadOptional.isPresent())
                .withFailMessage("Failed to serialize object")
                .isTrue();

        log.info("Payload: {}", payloadOptional.orElseThrow());

        var deserializedObject = TemporalConfigurationUtil.dataConverter()
                .fromPayload(payloadOptional.orElseThrow(), clazz, clazz);

        Assertions.assertThat(deserializedObject).isEqualTo(object);
    }

    private static Stream<Class<? extends BaseTemporalWorkflow.Id>> temporalIdClasses() {
        return new Reflections("")
                .getSubTypesOf(BaseTemporalWorkflow.Id.class)
                .stream()
                .filter(c -> !Modifier.isAbstract(c.getModifiers()));
    }

}

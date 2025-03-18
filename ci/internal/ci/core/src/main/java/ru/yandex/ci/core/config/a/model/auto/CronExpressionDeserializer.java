package ru.yandex.ci.core.config.a.model.auto;

import com.fasterxml.jackson.databind.util.StdConverter;
import org.springframework.scheduling.support.CronExpression;

public class CronExpressionDeserializer extends StdConverter<String, CronExpression> {
    @Override
    public CronExpression convert(String value) {
        return CronExpression.parse(value);
    }
}

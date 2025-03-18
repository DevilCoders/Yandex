package ru.yandex.ci.common.temporal;

import com.cronutils.builder.CronBuilder;
import com.cronutils.model.Cron;
import com.cronutils.model.CronType;
import com.cronutils.model.definition.CronDefinitionBuilder;
import com.cronutils.model.field.expression.FieldExpression;

public class TemporalCronUtils {

    private TemporalCronUtils() {
    }

    public static Cron everyMinute() {
        return CronBuilder.cron(CronDefinitionBuilder.instanceDefinitionFor(CronType.UNIX))
                .withMinute(FieldExpression.always())
                .withHour(FieldExpression.always())
                .withDoM(FieldExpression.always())
                .withMonth(FieldExpression.always())
                .withDoW(FieldExpression.always())
                .instance();
    }
}

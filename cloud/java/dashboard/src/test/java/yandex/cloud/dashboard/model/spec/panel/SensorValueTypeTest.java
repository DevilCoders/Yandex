package yandex.cloud.dashboard.model.spec.panel;

import org.junit.Assert;
import org.junit.Test;

import java.util.Set;

import static org.assertj.core.api.Assertions.assertThatExceptionOfType;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.DERIV;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.HIST_BUCKET;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.UseCase.VALUE;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.counter;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.rate;
import static yandex.cloud.dashboard.model.spec.panel.SensorValueType.raw;

/**
 * @author ssytnik
 */
public class SensorValueTypeTest {

    // https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/doc/TEMPLATES.md#общая-информцаия-по-параметрам-шаблонов
    @Test
    public void populateBuilder() {
        assertThatExceptionOfType(IllegalArgumentException.class).isThrownBy(() -> populateBuilder(raw, DERIV, ""));
        populateBuilder(rate, DERIV, "avg");
        populateBuilder(counter, DERIV, "max", "nn_deriv");

        populateBuilder(raw, VALUE, "sum");
        populateBuilder(rate, VALUE, "avg", "integrate_fn", "diff");
        populateBuilder(counter, VALUE, "max", "diff", "drop_below");

        populateBuilder(raw, HIST_BUCKET, "sum");
        populateBuilder(rate, HIST_BUCKET, "avg");
        populateBuilder(counter, HIST_BUCKET, "max", "nn_deriv");
    }

    private void populateBuilder(SensorValueType type, UseCase useCase, String downsamplingFn, String... selectFns) {
        Assert.assertEquals("downsampling functions mismatch", downsamplingFn, type.groupByTime(useCase).getFn());
        Assert.assertEquals("select function sets mismatch", Set.of(selectFns), type.selectBuilder(useCase).build().keySet());
    }

}
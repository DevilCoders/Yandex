package ru.yandex.monlib.metrics.encode.spack.format;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import ru.yandex.monlib.metrics.MetricType;

import static org.junit.Assert.assertEquals;

/**
 * @author Vladimir Gordiychuk
 */
@RunWith(Parameterized.class)
public class MetricTypesTest {
    @Parameterized.Parameter
    public MetricType type;
    @Parameterized.Parameter(1)
    public MetricValuesType valuesType;

    @Parameterized.Parameters(name = "{0}: {1}")
    public static List<Object[]> data() {
        MetricType[] types = MetricType.values();

        List<Object[]> pairs = new ArrayList<>();
        for (MetricValuesType valuesType : MetricValuesType.values()) {
            for (MetricType type : types) {
                pairs.add(new Object[]{type, valuesType});
            }
        }
        return pairs;
    }

    @Test
    public void packUnpack() {
        byte packed = MetricTypes.pack(type, valuesType);
        assertEquals(type, MetricTypes.metricType(packed));
        assertEquals(valuesType, MetricTypes.valuesType(packed));
    }
}

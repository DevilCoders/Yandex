package ru.yandex.monlib.metrics.encode.prometheus;

import java.io.StringWriter;

import javax.annotation.ParametersAreNonnullByDefault;

import org.junit.Assert;
import org.junit.Test;

import ru.yandex.monlib.metrics.histogram.Histograms;
import ru.yandex.monlib.metrics.labels.Labels;


/**
 * @author Oleg Baryshnikov
 */
@ParametersAreNonnullByDefault
public class ExtendedPrometheusWriterTest {
    @Test
    public void test() {
        StringWriter out = new StringWriter();
        try (ExtendedPrometheusWriter writer = new ExtendedPrometheusWriter(out)) {
            writer.writeGauge("dgauge_metric_nan", Labels.of(), Double.NaN);
            writer.writeGauge("dgauge_metric1", Labels.of(), 5.5);
            writer.writeGauge("dgauge_metric2", Labels.of("host", "cluster"), 4.8);
            writer.writeGauge("igauge_metric1", Labels.of(), 11);
            writer.writeGauge("igauge_metric2", Labels.of("host", "cluster"), 12);
            writer.writeCounter("counter_metric1", Labels.of(), 13);
            writer.writeCounter("counter_metric2", Labels.of("host", "cluster"), 14);
            writer.writeGauge("rate_metric1", Labels.of(), 2.5);
            writer.writeGauge("rate_metric2", Labels.of("host", "cluster"), 1);
            writer.writeHistogram("hist_metric_without_inf", Labels.of(), new double[]{1, 2, 5, 10}, new double[]{10.0, 2.0, 3.0, 0});
            writer.writeHistogram("hist_metric", Labels.of(), new double[]{1, 2, 5, 10, Histograms.INF_BOUND}, new double[]{10.0, 2.0, 3.0, 0, 1.0});
            writer.writeHistogram("hist_metric", Labels.of("host", "cluster"), new double[]{1, 2, 5, 10, Histograms.INF_BOUND}, new double[]{10.0, 2.0, 3.0, 0, 1.0});
        }
        String actual = out.toString();

        String expected = "" +
                "# TYPE dgauge_metric_nan gauge\n" +
                "dgauge_metric_nan NaN\n" +
                "# TYPE dgauge_metric1 gauge\n" +
                "dgauge_metric1 5.5\n" +
                "# TYPE dgauge_metric2 gauge\n" +
                "dgauge_metric2{host=\"cluster\"} 4.8\n" +
                "# TYPE igauge_metric1 gauge\n" +
                "igauge_metric1 11.0\n" +
                "# TYPE igauge_metric2 gauge\n" +
                "igauge_metric2{host=\"cluster\"} 12.0\n" +
                "# TYPE counter_metric1 counter\n" +
                "counter_metric1 13.0\n" +
                "# TYPE counter_metric2 counter\n" +
                "counter_metric2{host=\"cluster\"} 14.0\n" +
                "# TYPE rate_metric1 gauge\n" +
                "rate_metric1 2.5\n" +
                "# TYPE rate_metric2 gauge\n" +
                "rate_metric2{host=\"cluster\"} 1.0\n" +
                "# TYPE hist_metric_without_inf histogram\n" +
                "hist_metric_without_inf_bucket{le=\"1.0\"} 10.0\n" +
                "hist_metric_without_inf_bucket{le=\"2.0\"} 12.0\n" +
                "hist_metric_without_inf_bucket{le=\"5.0\"} 15.0\n" +
                "hist_metric_without_inf_bucket{le=\"10.0\"} 15.0\n" +
                "hist_metric_without_inf_bucket{le=\"+Inf\"} 15.0\n" +
                "hist_metric_without_inf_count 15.0\n" +
                "# TYPE hist_metric histogram\n" +
                "hist_metric_bucket{le=\"1.0\"} 10.0\n" +
                "hist_metric_bucket{le=\"2.0\"} 12.0\n" +
                "hist_metric_bucket{le=\"5.0\"} 15.0\n" +
                "hist_metric_bucket{le=\"10.0\"} 15.0\n" +
                "hist_metric_bucket{le=\"+Inf\"} 16.0\n" +
                "hist_metric_count 16.0\n" +
                "hist_metric_bucket{host=\"cluster\", le=\"1.0\"} 10.0\n" +
                "hist_metric_bucket{host=\"cluster\", le=\"2.0\"} 12.0\n" +
                "hist_metric_bucket{host=\"cluster\", le=\"5.0\"} 15.0\n" +
                "hist_metric_bucket{host=\"cluster\", le=\"10.0\"} 15.0\n" +
                "hist_metric_bucket{host=\"cluster\", le=\"+Inf\"} 16.0\n" +
                "hist_metric_count{host=\"cluster\"} 16.0\n\n";

        Assert.assertEquals(expected, actual);
    }
}

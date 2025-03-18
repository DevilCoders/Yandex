package ru.yandex.ci.storage.tests;

import java.util.stream.Stream;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.tests.tester.StorageTesterRegistrar;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.core.db.model.test.TestTag.YA_EXTERNAL;

public class MetricsTest extends StorageTestsYdbTestBase {

    @Test
    public void collectMetrics() {
        var registration = storageTester.register(
                registrar -> registrar.fullIteration()
                        .leftTask()
                        .rightTask(TASK_ID_RIGHT)
                        .rightTask(TASK_ID_RIGHT_TWO)
                        .rightTask(TASK_ID_RIGHT_THREE)
        );

        var testResult = exampleMediumTest(1L, 100L);

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_RIGHT)
                        .metric(seconds("time", 3))
                        .metric(bytes("mem", 105))
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .finish()
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_RIGHT_TWO)
                        .metric(seconds("time", 9))
                        .metric(bytes("mem", 400))
                        .metric(seconds("sleep_time", 6666))
        );

        // send same metric twice
        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_RIGHT_TWO)
                        .metric(seconds("time", 10))
                        .metric(bytes("mem", 367))
                        .metric(seconds("sleep_time", 8888))
                        .results(
                                testResult.toBuilder()
                                        .addTags(YA_EXTERNAL)
                                        .setTestStatus(Common.TestStatus.TS_XFAILED)
                                        .build()
                        )
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .finish()
        );

        storageTester.writeAndDeliver(
                registration,
                writer -> writer
                        .to(TASK_ID_RIGHT_THREE)
                        .metric(seconds("time", 8))
                        .metric(bytes("mem", 102))
                        .testTypeFinish(Actions.TestType.CONFIGURE)
                        .finish()
        );

        var iteration = storageTester.getIteration(registration.getIteration().getId());

        var metricValues = iteration.getStatistics().getMetrics().getValues();
        assertThat(metricValues).containsOnlyKeys("time", "sleep_time", "mem");

        assertThat(metricValues.get("time").getValue()).isEqualTo(3 + 10 /* max(9,10) */ + 8);
        assertThat(metricValues.get("mem").getValue()).isEqualTo(400);
        assertThat(metricValues.get("sleep_time").getValue()).isEqualTo(8888);
    }

    @Test
    public void restarts() {
        var registrationOne = storageTester.register(
                registrar -> registrar.fullIteration()
                        .leftTask(TASK_ID_LEFT)
                        .rightTask(TASK_ID_RIGHT)
        );

        var registrationTwo = storageTester.register(
                registrar -> registrar.fullIteration()
                        .leftTask(TASK_ID_LEFT_TWO)
                        .rightTask(TASK_ID_RIGHT_TWO)
        );

        sendMetrics(registrationOne, TASK_ID_LEFT, seconds("time", 2), bytes("mem", 101));
        sendMetrics(registrationOne, TASK_ID_RIGHT, seconds("time", 7), bytes("mem", 98));
        sendMetrics(registrationTwo, TASK_ID_LEFT_TWO, seconds("time", 5), bytes("mem", 107));
        sendMetrics(registrationTwo, TASK_ID_RIGHT_TWO, seconds("time", 3), bytes("mem", 94));

        var metaId = registrationOne.getIteration().getId().toMetaId();
        var iteration1 = storageTester.getIteration(registrationOne.getIteration().getId());
        var iteration2 = storageTester.getIteration(registrationTwo.getIteration().getId());
        var metaIteration = storageTester.getIteration(metaId);

        var metrics1 = iteration1.getStatistics().getMetrics().getValues();
        var metrics2 = iteration2.getStatistics().getMetrics().getValues();
        var metricsMeta = metaIteration.getStatistics().getMetrics().getValues();

        assertThat(metrics1.get("time").getValue()).isEqualTo(2 + 7);
        assertThat(metrics2.get("time").getValue()).isEqualTo(5 + 3);
        assertThat(metricsMeta.get("time").getValue()).isEqualTo(2 + 7 + 5 + 3);

        assertThat(metrics1.get("mem").getValue()).isEqualTo(101);
        assertThat(metrics2.get("mem").getValue()).isEqualTo(107);
        assertThat(metricsMeta.get("mem").getValue()).isEqualTo(107);
    }

    private void sendMetrics(
            StorageTesterRegistrar.RegistrationResult registration, String taskId, Common.Metric... metrics
    ) {
        storageTester.writeAndDeliver(
                registration,
                writer -> {
                    writer.to(taskId);
                    Stream.of(metrics).forEach(writer::metric);
                    writer.finish();
                }
        );
    }

    @SuppressWarnings("SameParameterValue")
    private static Common.Metric seconds(String name, int value) {
        return Common.Metric.newBuilder()
                .setName(name)
                .setValue(value)
                .setAggregate(Common.MetricAggregateFunction.SUM)
                .setSize(Common.MetricSize.SECONDS)
                .build();
    }

    @SuppressWarnings("SameParameterValue")
    private static Common.Metric bytes(String name, int value) {
        return Common.Metric.newBuilder()
                .setName(name)
                .setValue(value)
                .setAggregate(Common.MetricAggregateFunction.MAX)
                .setSize(Common.MetricSize.BYTES)
                .build();
    }

}

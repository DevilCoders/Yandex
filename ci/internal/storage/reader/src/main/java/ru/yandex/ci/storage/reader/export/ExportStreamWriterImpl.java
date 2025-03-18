package ru.yandex.ci.storage.reader.export;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.stream.Collectors;

import com.google.common.collect.Lists;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.converter.TskvConverter;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageRawBytesWriter;

@Slf4j
public class ExportStreamWriterImpl extends StorageRawBytesWriter implements ExportStreamWriter {
    private static final int MAX_NUMBER_OF_LINES_IN_MESSAGE = 2048;

    private final Random random;
    private final TskvConverter tskvConverter;

    public ExportStreamWriterImpl(
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            TskvConverter tskvConverter,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);

        random = new Random();
        this.tskvConverter = tskvConverter;
    }

    @Override
    public void onAutocheckTestResults(List<CheckTaskTestResult> testResults) {
        log.info("Received for export stream write {} test results", testResults.size());

        var testMetricsData = collectTestMetrics(testResults).entrySet().stream()
                .map(e -> {
                    var line = new LinkedHashMap<String, String>();
                    line.put("dateTime", e.getKey().getDateTime().toString());
                    line.put("timestampSeconds", Long.toString(e.getKey().getDateTime().getEpochSecond()));
                    line.put("checkType", e.getKey().getCheckType().toString());
                    line.put("jobName", e.getKey().getJobName());
                    line.put("right", e.getKey().getRight().toString());
                    line.put("partition", e.getKey().getPartition().toString());
                    line.put("type", e.getKey().getType().toString());
                    line.put("status", e.getKey().getStatus().toString());
                    line.put("toolchain", e.getKey().getToolchain());
                    line.put("checkId", e.getKey().getCheckId().toString());
                    line.put("iterationType", e.getKey().getIterationType().toString());
                    line.put("iterationNumber", e.getKey().getIterationNumber().toString());
                    line.put("count", e.getValue().toString());

                    return line;
                })
                .collect(Collectors.toList());

        log.info("Writing to export stream {} test metrics after aggregation", testMetricsData.size());
        writeWithRetries(toMessages(testMetricsData));
    }

    private Map<TestMetricsKey, Long> collectTestMetrics(List<CheckTaskTestResult> testResults) {
        var time = Instant.now().truncatedTo(ChronoUnit.MINUTES);
        return testResults.stream().collect(
                Collectors.groupingBy(
                        m -> new TestMetricsKey(
                                time,
                                m.getCheck().getType(),
                                m.getTask().getJobName(),
                                m.getTask().isRight(),
                                m.getPartition(),
                                m.getTestResult().getResultType(),
                                m.getTestResult().getTestStatus(),
                                m.getTestResult().getId().getToolchain(),
                                m.getTask().getId().getIterationId().getCheckId().getId(),
                                m.getTask().getId().getIterationId().getIterationType(),
                                m.getTask().getId().getIterationId().getNumber()
                        ),
                        Collectors.counting()
                )
        );
    }

    private List<StorageRawBytesWriter.Message> toMessages(List<LinkedHashMap<String, String>> data) {
        return Lists.partition(data, MAX_NUMBER_OF_LINES_IN_MESSAGE).stream()
                .map(tskvConverter::convertLines)
                .map(d -> new Message(getPartition(), d.getBytes(StandardCharsets.UTF_8)))
                .collect(Collectors.toList());
    }

    private int getPartition() {
        return random.nextInt(numberOfPartitions);
    }

    @Value
    private static class TestMetricsKey {
        Instant dateTime;
        CheckOuterClass.CheckType checkType;
        String jobName;
        Boolean right;
        Integer partition;
        Common.ResultType type;
        Common.TestStatus status;
        String toolchain;
        Long checkId;
        CheckIteration.IterationType iterationType;
        Integer iterationNumber;
    }
}

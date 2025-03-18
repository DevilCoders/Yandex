package ru.yandex.ci.client.logs;

import java.io.File;
import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Map;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.SystemUtils;

import ru.yandex.ci.client.tvm.TvmClientWrapper;
import ru.yandex.ci.client.tvm.grpc.TvmCallCredentials;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.passport.tvmauth.TvmClient;


@Slf4j
public class LogsClientManualRunner {

    private LogsClientManualRunner() {
    }

    public static LogsClient createLogsClient() {

        int selfTvmId = 2018960;

        String tvmSecret;
        try {
            tvmSecret = Files.asCharSource(
                    new File(SystemUtils.getUserHome(), "/.ci/tvm-secret"),
                    Charsets.UTF_8
            ).read();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        int logsTvmId = 2018960;
        TvmClient tvmClient = TvmClientWrapper.getTvmClient(
                selfTvmId,
                tvmSecret,
                new int[]{logsTvmId},
                "PROD_YATEAM"
        );

        GrpcClientProperties properties = GrpcClientProperties.builder()
                .endpoint("query.logs.yandex-team.ru:443")
                .deadlineAfter(Duration.ofMinutes(2))
                .userAgent("CI")
                .plainText(false)
                .callCredentials(new TvmCallCredentials(tvmClient, logsTvmId))
                .build();


        return LogsClient.create(properties);
    }

    public static void main(String[] args) throws Exception {

        var client = createLogsClient();

        var to = Instant.now();
        var from = to.minus(1, ChronoUnit.DAYS);

        var query = Map.of(
                "project", "ci",
                "service", "ci-tms",
                "task_id", "FlowJobWorkflow-fc3c1151f8c3286b060a08bc187aac00341e13cc1c0543be18277f60d823e481-full" +
                        "-curcuit-left-jobs-1-1"
        );

        var logs = client.getLog(from, to, query, 10000);
        log.info("Found {} log records", logs.size());

    }

}

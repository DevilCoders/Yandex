package ru.yandex.ci.tools.flows;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.db.bulk.BulkParams;
import ru.yandex.ci.core.pr.PullRequestDiffSet.ByPullRequestId;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(YdbCiConfig.class)
@Configuration
public class UpdatePullRequestDiffSetByPullRequestId extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Override
    protected void run() throws IOException {

        var lines = Files.readAllLines(Path.of("/Users/miroslav2/jdbc_SCAN.csv"));
        var ctx = new Object() {
            int total;

            void report() {
                log.info("Saved {} records", total);
            }
        };

        long started = System.currentTimeMillis();
        for (var list : Lists.partition(lines, 10000)) {
            db.tx(() -> {
                var table = db.pullRequestDiffSetTable().byPullRequestId();
                var records = list.stream()
                        .map(idStr -> StringUtils.strip(idStr, "\""))
                        .filter(idStr -> !idStr.isEmpty())
                        .map(Long::parseLong)
                        .map(ByPullRequestId.Id::of)
                        .map(ByPullRequestId::of)
                        .toList();
                table.bulkUpsert(records, BulkParams.DEFAULT);
            });
            ctx.total += list.size();
            ctx.report();
        }
        log.info("Complete in {} msec", System.currentTimeMillis() - started);
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}

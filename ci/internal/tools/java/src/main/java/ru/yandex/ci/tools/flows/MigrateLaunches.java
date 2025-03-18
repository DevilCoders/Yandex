package ru.yandex.ci.tools.flows;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import com.google.common.collect.Lists;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.io.FileUtils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Configuration
@Import(YdbCiConfig.class)
public class MigrateLaunches extends AbstractSpringBasedApp {

    @Autowired
    CiDb db;

    @Override
    protected void run() throws IOException {
        var lines = FileUtils.readLines(new File("/Users/miroslav2/IdeaProjects/arcadia-ci/jdbc_SCAN.tsv"), "UTF-8");

        var launches = lines.stream()
                .map(line -> {
                    var parts = line.split("\t", 2);
                    if (parts.length != 2) {
                        log.info("Skip {}", line);
                        return null;
                    } else {
                        return parts;
                    }
                })
                .filter(Objects::nonNull)
                .map(parts -> Launch.Id.of(parts[0], Integer.parseInt(parts[1])))
                .peek(launchId -> log.info("Processing launch: {}", launchId))
                .collect(Collectors.toList());


        for (var list : Lists.partition(launches, 100)) {
            db.currentOrTx(() -> {
                var found = db.launches().find(Set.copyOf(list));
                for (var launch : found) {
                    var rollbackFlows = launch.getFlowInfo().getRollbackFlows();
                    if (rollbackFlows.isEmpty()) {
                        log.warn("ROLLBACK FLOWS are empty, skip {}", launch.getId());
                    } else {
                        db.launches().save(launch.toBuilder()
                                .flowInfo(launch.getFlowInfo().toBuilder().rollbackFlows(List.of()).build())
                                .build());
                        log.warn("Updating {}", launch.getId());
                    }
                }
            });
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }

}


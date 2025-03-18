package ru.yandex.ci.tools.testenv;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;

import com.beust.jcommander.Parameter;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVPrinter;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.spring.clients.TestenvClientConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.tms.metric.ci.TestenvUsageMetricCronTask;
import ru.yandex.ci.tms.metric.ci.TestenvUsageMetricCronTask.Counters;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(CountTeProjects.Config.class)
@Configuration
public class CountTeProjects extends AbstractSpringBasedApp {

    @Parameter(names = "--output-path")
    private Path outputPath;

    @Autowired
    private TestenvUsageMetricCronTask testenvUsageMetricCronTask;

    @Override
    protected void run() throws Exception {
        var projects = testenvUsageMetricCronTask.countProjects();
        log.info("Loaded {} projects", projects.size());
        dump(projects);
    }

    private void dump(Map<String, Counters> projects) throws IOException {
        var sb = new StringBuilder();
        try (var printer = new CSVPrinter(sb, CSVFormat.DEFAULT.withHeader(
                "project_name",
                "projects",
                "active_groups",
                "active_jobs",
                "active_pre_commit_jobs",
                "disabled_jobs",
                "autocheck_branch",
                "custom_branch",
                "release_machine_jobs"
        ))) {

            for (var project : projects.entrySet()) {
                var projectName = project.getKey();
                var counters = project.getValue();
                printer.printRecord(
                        projectName,
                        counters.projects,
                        counters.activeGroups,
                        counters.activeJobs,
                        counters.activePreCommitJobs,
                        counters.disabledJobs,
                        counters.autocheckBranch,
                        counters.customBranch,
                        counters.releaseMachineJobs
                );
            }
        }

        Files.writeString(outputPath, sb);
        log.info("Dumped to {}", outputPath);
    }


    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

    @Import({
            TestenvClientConfig.class,
            YdbCiConfig.class
    })
    @Configuration
    public static class Config {
        @Bean
        public TestenvUsageMetricCronTask testenvUsageMetricCronTask(
                CiMainDb db,
                TestenvClient testenvClient
        ) {
            return new TestenvUsageMetricCronTask(db, testenvClient, null);
        }
    }
}

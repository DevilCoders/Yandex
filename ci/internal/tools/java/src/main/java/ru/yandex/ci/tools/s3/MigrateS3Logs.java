package ru.yandex.ci.tools.s3;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import com.amazonaws.services.s3.model.S3ObjectSummary;
import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.S3LogStorage;
import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.flow.engine.runtime.bazinga.FlowTask;
import ru.yandex.ci.tools.AbstractSpringBasedApp;
import ru.yandex.commune.bazinga.impl.FullJobId;

@Slf4j
@Import(S3LogStorageConfig.class)
@Configuration
public class MigrateS3Logs extends AbstractSpringBasedApp {

    @Autowired
    AmazonS3 aws;

    @Autowired
    S3LogStorage s3;

    @Value("${migrate.s3.bucket:ci-bazinga}")
    String bucketName;

    @Value("${migrate.s3.env:stable}")
    String env;

    @Value("${migrate.s3.dayThreads:16}")
    int dayThreads;

    @Value("${migrate.s3.taskThreads:256}")
    int taskThreads;

    ExecutorService daysExecutor;
    ExecutorService taskExecutor;

    @Override
    protected void run() throws Exception {
        tryS3();
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    void tryS3() throws InterruptedException, ExecutionException {
        daysExecutor = Executors.newFixedThreadPool(dayThreads);
        taskExecutor = Executors.newFixedThreadPool(taskThreads);

        s3 = s3.withEnv(env);

        migrateTasks(true);
        migrateTasks(false);

        taskExecutor.shutdown();
        taskExecutor.awaitTermination(15, TimeUnit.MINUTES);

        daysExecutor.shutdown();
        daysExecutor.awaitTermination(15, TimeUnit.MINUTES);
    }

    private void migrateTasks(boolean onetime) throws ExecutionException, InterruptedException {
        s3.setOnetimeTaskChecker(taskId ->
                onetime
                        ? Optional.of(Mockito.mock(FlowTask.class))
                        : Optional.empty()
        );

        var prefix = "%s/%s/".formatted(env, onetime ? "onetime" : "cron");
        log.info("Checking {}", prefix);

        for (var dir : aws.listObjectsV2(listRequest(prefix)).getCommonPrefixes()) {
            migrateSingleTask(dir);
        }
    }

    private void migrateSingleTask(String prefix) throws ExecutionException, InterruptedException {
        var cleanPrefix = StringUtils.removeEnd(prefix, "/");

        var lastPrefix = cleanPrefix.lastIndexOf('/');
        var taskName = cleanPrefix.substring(lastPrefix + 1);

        log.info("Checking {}, task {}", prefix, taskName);

        var tasks = aws.listObjectsV2(listRequest(prefix)).getCommonPrefixes().stream()
                .map(dir -> daysExecutor.submit(() -> migrateSingleTaskDay(taskName, dir)))
                .toList();
        for (var task : tasks) {
            task.get();
        }
    }

    private void migrateSingleTaskDay(String taskName, String prefix) {
        log.info("Checking {}", prefix);
        for (var dir : aws.listObjectsV2(listRequest(prefix)).getCommonPrefixes()) {
            migrateSingleTaskDayHour(taskName, dir);
        }
    }

    private void migrateSingleTaskDayHour(String taskName, String dir) {
        int allFiles = 0;
        List<Future<?>> allTasks = new ArrayList<>(1024);
        String nextContinuationToken = null;
        while (true) {
            var request = listRequest(dir);
            request.setContinuationToken(nextContinuationToken);

            var response = aws.listObjectsV2(request);
            var files = response.getObjectSummaries();

            allFiles += files.size();
            if (files.isEmpty()) {
                break;
            }

            var tasks = files.stream()
                    .map(file -> migrateSingleFile(taskName, dir, file))
                    .filter(Objects::nonNull)
                    .toList();

            allTasks.addAll(tasks);

            if (response.isTruncated()) {
                nextContinuationToken = response.getNextContinuationToken();
            } else {
                break;
            }
        }

        log.info("[{}] Moving {} out of {} files", dir, allTasks.size(), allFiles);
        for (var task : allTasks) {
            try {
                task.get();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    @Nullable
    private Future<?> migrateSingleFile(String taskName, String dir, S3ObjectSummary file) {
        log.debug("Checking file: {}", file.getKey());

        var name = file.getKey().substring(dir.length());

        Preconditions.checkState(name.endsWith(".log.lz4"));
        name = StringUtils.removeEnd(name, ".log.lz4");

        int hostPoint = name.lastIndexOf('.');
        Preconditions.checkState(hostPoint > 0);

        var job = name.substring(0, hostPoint).replace('_', '/');
        var host = name.substring(hostPoint + 1);

        var fullJobId = taskName + "/" + job;
        log.debug("Parsing {}", fullJobId);

        var jobId = FullJobId.parse(fullJobId);
        var location = s3.getLocation(host, jobId);

        log.debug("Location: {}", location);

        if (file.getKey().equals(location.getS3Key())) {
            return null; // --- unchanged
        }

        return taskExecutor.submit(() -> {
            log.debug("Moving {} -> {}", file.getKey(), location.getS3Key());
            aws.copyObject(
                    bucketName, file.getKey(),
                    bucketName, location.getS3Key());
            aws.deleteObject(bucketName, file.getKey());
        });
    }

    private ListObjectsV2Request listRequest(String prefix) {
        var request = new ListObjectsV2Request();
        request.setBucketName(bucketName);
        request.setPrefix(prefix);
        request.setDelimiter("/");
        return request;
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}

package ru.yandex.ci.tools.s3;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(S3LogStorageConfig.class)
@Configuration
public class MoveS3Files extends AbstractSpringBasedApp {

    @Autowired
    AmazonS3 aws;

    List<String> directories = List.of(
            cron("archiveCron"),
            cron("checkIdGenerationCron"),
            cron("groupsSyncCron"),
            cron("iterationTimeoutCron"),
            cron("missingRevisionsCron"),
            cron("muteDigestCron"),
            cron("oldCiMuteCron"),
            cron("statisticsCron"),
            onetime("arcanumCheckStatusReporter"),
            onetime("archive"),
            onetime("badgeEventSend"),
            onetime("cancelIterationFlow"),
            onetime("exportToClickhouse"),
            onetime("largeAutostartBootstrap"),
            onetime("largeFlow"),
            onetime("largeLogbroker"),
            onetime("markDiscoveredCommit"),
            onetime("restartTests")
    );

    String bucketFrom = "ci-bazinga";
    String bucketTo = "ci-storage-bazinga";

    int threads = 64;

    ExecutorService executor;

    @Override
    protected void run() throws Exception {
        tryS3();
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    void tryS3() throws InterruptedException, ExecutionException {
        executor = Executors.newFixedThreadPool(threads);

        for (var dir : directories) {
            moveDir(dir);
        }

        executor.shutdown();
        executor.awaitTermination(15, TimeUnit.MINUTES);
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    private void moveDir(String dir) throws ExecutionException, InterruptedException {
        log.info("Moving {}", dir);

        var allFutures = new ArrayList<CompletableFuture<?>>();

        for (var day : aws.listObjectsV2(listRequest(dir)).getCommonPrefixes()) {
            var dayFutures = new ArrayList<CompletableFuture<?>>();
            for (var time : aws.listObjectsV2(listRequest(day)).getCommonPrefixes()) {

                String nextToken = null;
                while (true) {
                    var request = listRequest(time);
                    request.setContinuationToken(nextToken);

                    var response = aws.listObjectsV2(request);

                    var files = response.getObjectSummaries();
                    if (files.isEmpty()) {
                        break;
                    }

                    for (var file : files) {
                        var future = CompletableFuture.runAsync(() -> moveFile(file.getKey()), executor);
                        dayFutures.add(future);
                        allFutures.add(future);
                    }

                    if (response.isTruncated()) {
                        nextToken = response.getNextContinuationToken();
                    } else {
                        break;
                    }
                }
            }
            CompletableFuture.allOf(dayFutures.toArray(CompletableFuture[]::new))
                    .thenRun(() -> log.info("{} complete ({} files)", day, dayFutures.size()));
        }

        CompletableFuture.allOf(allFutures.toArray(CompletableFuture[]::new)).get();
        log.info("{} moved ({} files)", dir, allFutures.size());
    }

    private void moveFile(String key) {
        aws.copyObject(bucketFrom, key, bucketTo, key);
        aws.deleteObject(bucketFrom, key);
    }

    private ListObjectsV2Request listRequest(String prefix) {
        var request = new ListObjectsV2Request();
        request.setBucketName(bucketFrom);
        request.setPrefix(prefix);
        request.setDelimiter("/");
        return request;
    }

    private static String cron(String dir) {
        return "stable/cron/" + dir + "/";
    }

    private static String onetime(String dir) {
        return "stable/onetime/" + dir + "/";
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }
}

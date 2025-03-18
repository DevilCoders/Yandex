package ru.yandex.ci.tools.s3;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ListObjectsV2Request;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import ru.yandex.ci.common.bazinga.spring.S3LogStorageConfig;
import ru.yandex.ci.tools.AbstractSpringBasedApp;

@Slf4j
@Import(S3LogStorageConfig.class)
@Configuration
public class DeleteS3Files extends AbstractSpringBasedApp {

    private static final ThreadLocal<DecimalFormat> FMT = ThreadLocal.withInitial(() -> new DecimalFormat("#0.00"));

    @Autowired
    AmazonS3 aws;

    List<String> directories = List.of(onetime("flow"));

    String olderThan = "20220901";

    String bucket = "ci-bazinga";

    long checkFileSize = 50 * 1024 * 1024;

    boolean dryRun = false;

    int dirThreads = 16;
    int fileThreads = 64;

    ExecutorService dirExecutor;
    ExecutorService fileExecutor;

    @Override
    protected void run() throws Exception {
        tryS3();
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    void tryS3() throws InterruptedException, ExecutionException {
        dirExecutor = Executors.newFixedThreadPool(dirThreads);
        fileExecutor = Executors.newFixedThreadPool(fileThreads);

        for (var dir : directories) {
            deleteDir(dir);
        }

        fileExecutor.shutdown();
        fileExecutor.awaitTermination(15, TimeUnit.MINUTES);

        dirExecutor.shutdown();
        dirExecutor.awaitTermination(15, TimeUnit.MINUTES);
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    private void deleteDir(String dir) throws ExecutionException, InterruptedException {
        log.info("Deleting {} older than {}", dir, olderThan);


        var globalState = new State();
        var allDirs = new ArrayList<CompletableFuture<?>>();

        for (var day : aws.listObjectsV2(listRequest(dir)).getCommonPrefixes()) {
            if (day.compareTo(dir + olderThan) >= 0) {
                log.info("Skip {}", day);
                continue; // ---
            }

            var dirFuture = CompletableFuture.runAsync(() -> deleteDir(day, globalState), dirExecutor);
            allDirs.add(dirFuture);
        }

        CompletableFuture.allOf(allDirs.toArray(CompletableFuture[]::new)).get();

        log.info("{} {} ({})", dir, dryRun ? "tested" : "deleted", globalState);
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    private void deleteDir(String day, State globalState) {
        var currentState = new State();
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
                    var size = file.getSize();
                    currentState.addFile(size);
                    globalState.addFile(size);
                    if (size > checkFileSize) {
                        var future = CompletableFuture.runAsync(() -> {
                            currentState.addCleanupFile(size);
                            globalState.addCleanupFile(size);

                            deleteFile(file.getKey());
                        }, fileExecutor);
                        dayFutures.add(future);
                    }
                }

                if (response.isTruncated()) {
                    nextToken = response.getNextContinuationToken();
                } else {
                    break;
                }
            }
        }
        CompletableFuture.allOf(dayFutures.toArray(CompletableFuture[]::new))
                .thenRun(() -> log.info("{} {} ({})", day, dryRun ? "tested" : "deleted", currentState));
    }

    private void deleteFile(String key) {
        if (!dryRun) {
            aws.deleteObject(bucket, key);
        }
    }

    private ListObjectsV2Request listRequest(String prefix) {
        var request = new ListObjectsV2Request();
        request.setBucketName(bucket);
        request.setPrefix(prefix);
        request.setDelimiter("/");
        return request;
    }

    static String cron(String dir) {
        return "stable/cron/" + dir + "/";
    }

    static String onetime(String dir) {
        return "stable/onetime/" + dir + "/";
    }

    private static String toMib(long bytes) {
        return FMT.get().format(bytes / (1024.0 * 1024.0));
    }

    public static void main(String[] args) {
        startAndStopThisClass(args);
    }


    @Value
    private static class State {
        AtomicLong sizes = new AtomicLong();
        AtomicLong files = new AtomicLong();
        AtomicLong cleanupSizes = new AtomicLong();
        AtomicLong cleanupFiles = new AtomicLong();

        void addFile(long size) {
            sizes.addAndGet(size);
            files.incrementAndGet();
        }

        void addCleanupFile(long size) {
            cleanupSizes.addAndGet(size);
            cleanupFiles.incrementAndGet();
        }

        @Override
        public String toString() {
            return "%s out of %s files clean, %s out of %s MiB clean".formatted(
                    cleanupFiles.get(), files.get(), toMib(cleanupSizes.get()), toMib(sizes.get())
            );
        }
    }
}

package ru.yandex.ci.common.bazinga;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.ThreadContext;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.LoggerContext;
import org.apache.logging.log4j.core.appender.FileAppender;
import org.apache.logging.log4j.core.config.Configuration;
import org.apache.logging.log4j.core.config.LoggerConfig;
import org.apache.logging.log4j.core.filter.ThreadContextMapFilter;
import org.apache.logging.log4j.core.layout.PatternLayout;
import org.apache.logging.log4j.core.util.KeyValuePair;

import ru.yandex.bolts.function.Function1V;
import ru.yandex.ci.common.bazinga.S3LogStorage.LogLocation;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.commune.bazinga.impl.worker.LogDirManager;
import ru.yandex.commune.bazinga.impl.worker.WorkerTask;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLogger;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskLoggerSource;
import ru.yandex.misc.io.file.File2;
import ru.yandex.misc.log.reqid.RequestIdStack;
import ru.yandex.misc.thread.ThreadUtils;

@Slf4j
@RequiredArgsConstructor
public class S3WorkerTaskLoggerSource implements WorkerTaskLoggerSource, Closeable {
    @Nonnull
    private final S3LogStorage s3LogStorage;

    @Nonnull
    private final ExecutorService executor;

    private final boolean immediateFlush;

    @Override
    public void close() {
        executor.shutdown();
        try {
            if (!executor.awaitTermination(15, TimeUnit.SECONDS)) {
                log.warn("Unable to wait for termination");
            }
        } catch (Exception e) {
            log.error("Unable to wait for executor termination", e);
        }
    }

    @Override
    public WorkerTaskLogger createWorkerTaskLogger(LogDirManager logDirManager) {
        return new S3WorkerTaskLogger();
    }

    private void scheduleUpload(LogLocation location) throws IOException {
        var path = location.getLocalPath();
        if (!Files.exists(path)) {
            log.info("Log file [{}] does not exists, nothing to upload", path);
            return;
        }
        if (Files.size(path) == 0) {
            log.info("Log file [{}] is empty, nothing to upload", path);
            return;
        }

        executor.execute(() -> {
            try {
                s3LogStorage.compressAndUpload(location);
            } catch (Exception io) {
                log.error("Unable to save file into S3: {}", path, io);
            }
        });
    }

    public class S3WorkerTaskLogger implements WorkerTaskLogger {

        private final Configuration contextConfig;
        private final LoggerConfig loggerConfig;

        {
            LoggerContext ctx = (LoggerContext) LogManager.getContext(false);
            contextConfig = ctx.getConfiguration();
            loggerConfig = contextConfig.getLoggerConfig(LogManager.ROOT_LOGGER_NAME);
        }

        @Override
        public void runTaskWithLogger(WorkerTask task, Function1V<File2> runWithLog) {
            var hostname = HostnameUtils.getHostname();
            var location = s3LogStorage.getLocation(hostname, task.getFullJobId());
            log.info("Task [{}]] has location: {}", task.getFullJobId(), location);

            try (var appender = new LoggingAppender(location)) {
                ThreadContext.put("BAZINGA", "true");
                runWithLog.accept(new File2(appender.getFile()));
            } catch (IOException e) {
                log.error("Unable to write logs", e);
            } finally {
                ThreadContext.remove("BAZINGA");
            }
        }

        class LoggingAppender implements Closeable {
            private final LogLocation location;
            private final FileAppender appender;

            private LoggingAppender(@Nonnull LogLocation location) throws IOException {
                this.location = location;

                var localPath = location.getLocalPath();
                Files.createDirectories(localPath.getParent());

                var stringPath = localPath.toString();
                log.info("Saving logs into {}", stringPath);

                var layout = PatternLayout.newBuilder()
                        .withConfiguration(contextConfig)
                        .withPattern("%d %-5p [%X{tx}] [%c{1}] %m%n")
                        .build();

                this.appender = FileAppender.newBuilder()
                        .setLayout(layout)
                        .setName(ThreadUtils.escapeForThreadName(stringPath))
                        .setIgnoreExceptions(true)
                        .setConfiguration(contextConfig)
                        .withFileName(stringPath)
                        .withAppend(true)
                        .setImmediateFlush(immediateFlush)
                        .setBufferedIo(true)
                        .setBufferSize(8192)
                        .build();

                appender.start();

                var filterValue = "#rid=" + RequestIdStack.current().get();
                var filter = ThreadContextMapFilter.createFilter(
                        new KeyValuePair[]{new KeyValuePair("ndc", filterValue)},
                        null,
                        Filter.Result.ACCEPT,
                        Filter.Result.DENY
                );
                filter.start();
                loggerConfig.addAppender(appender, Level.DEBUG, filter);
            }

            private File getFile() {
                return location.getLocalPath().toFile();
            }

            @Override
            public void close() throws IOException {
                loggerConfig.removeAppender(appender.getName());
                appender.stop();

                scheduleUpload(location);
            }
        }
    }

}

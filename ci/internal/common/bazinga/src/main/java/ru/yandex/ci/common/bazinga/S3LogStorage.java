package ru.yandex.ci.common.bazinga;

import java.io.IOException;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.time.Duration;
import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.function.Function;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.S3Object;
import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.With;
import lombok.extern.slf4j.Slf4j;
import net.jpountz.lz4.LZ4FrameInputStream;
import net.jpountz.lz4.LZ4FrameOutputStream;
import org.apache.commons.lang3.StringUtils;
import org.springframework.util.StreamUtils;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.LogStorage;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.misc.io.InputStreamSource;
import ru.yandex.misc.io.SimpleInputStreamSource;
import ru.yandex.misc.io.file.FileInputStreamSource;

@SuppressWarnings("ReferenceEquality")
@Slf4j
@RequiredArgsConstructor
@AllArgsConstructor
public class S3LogStorage implements LogStorage {

    private static final DateTimeFormatter FMT_DATE = utcFormat("yyyyMMdd");
    private static final DateTimeFormatter FMT_TIME = utcFormat("HH");

    private static final String ONETIME_CLASS = "onetime";
    private static final String CRON_CLASS = "cron";

    private static final String EXT_LOG = ".log";
    private static final String EXT_LZ4 = ".lz4";
    private static final String EXT_TMP = ".tmp";

    @Nonnull
    private final AmazonS3 amazonS3;

    @Nonnull
    private final String bucket;

    @Nonnull
    @With
    private final String env;

    @Nonnull
    private final String workingDir;

    @Nullable
    private Function<TaskId, Optional<OnetimeTask>> onetimeTaskChecker;

    public void setOnetimeTaskChecker(@Nonnull Function<TaskId, Optional<OnetimeTask>> onetimeTaskChecker) {
        this.onetimeTaskChecker = onetimeTaskChecker;
    }

    public void cleanup(Duration olderThan, Instant now) {
        var todayDir = FMT_DATE.format(now);
        var compareWith = now.minusMillis(olderThan.toMillis());

        log.info("Cleaning files older than {} ({}), keeping directory from {}", olderThan, compareWith, todayDir);
        log.info("Starting from {}", workingDir);

        try {
            Files.walkFileTree(Path.of(workingDir), new SimpleFileVisitor<>() {
                boolean acceptDir;
                boolean lastModifiedWarned;
                final Map<Path, String> checkDirs = new HashMap<>();
                final Map<String, LogRecord> filesToRemove = new HashMap<>();

                @Override
                public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) throws IOException {
                    acceptDir = false;
                    filesToRemove.clear();
                    try {
                        var dirName = dir.getFileName().toString();
                        FMT_DATE.parse(dirName);
                        checkDirs.put(dir, dirName);
                        acceptDir = true;
                    } catch (Exception e) {
                        // not a date
                    }
                    return super.preVisitDirectory(dir, attrs);
                }

                @Override
                public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                    if (acceptDir) {
                        var name = file.getFileName().toString();
                        if (name.endsWith(EXT_LOG)) {
                            var lastModified = attrs.lastModifiedTime().toInstant();
                            if (isOldFile(lastModified)) {
                                var rec = filesToRemove.computeIfAbsent(name, n -> new LogRecord());
                                rec.setLogFile(file);
                                rec.setLastLogModified(attrs.lastModifiedTime().toInstant());
                            }
                        } else if (name.endsWith(EXT_LZ4)) {
                            var sourceName = StringUtils.removeEnd(name, EXT_LZ4);
                            filesToRemove.computeIfAbsent(sourceName, n -> new LogRecord())
                                    .setCompressedFile(file);
                        } else if (name.endsWith(EXT_TMP)) {
                            var lastModified = attrs.lastModifiedTime().toInstant();
                            if (isOldFile(lastModified)) {
                                Files.delete(file);
                            }
                        }
                    }
                    return super.visitFile(file, attrs);
                }

                @Override
                public FileVisitResult postVisitDirectory(Path dir, IOException exc) throws IOException {
                    var currentDir = checkDirs.get(dir);
                    if (currentDir != null) {
                        for (var rec : filesToRemove.values()) {
                            if (rec.getLogFile() == null || rec.getCompressedFile() == null) {
                                tryToFixUpload(rec);
                                continue; // ---
                            }
                           deleteProcessedFile(rec);
                        }

                        if (currentDir.compareTo(todayDir) < 0) {
                            try {
                                Files.delete(dir);
                                log.info("Cleaning empty directory: {}", dir);
                            } catch (IOException io) {
                                log.info("Cannot clean directory: {}", dir);
                            }
                        }
                    }
                    return super.postVisitDirectory(dir, exc);
                }

                private boolean isOldFile(Instant lastModified) {
                    if (lastModified.toEpochMilli() == 0 && !lastModifiedWarned) {
                        log.warn("Looks like filesystem does not support last modification attributes");
                        lastModifiedWarned = true;
                        return false;
                    }
                    return lastModified.isBefore(compareWith);
                }

                private void tryToFixUpload(LogRecord rec) throws IOException {
                    var logFile = rec.getLogFile();
                    if (logFile == null) {
                        log.warn("Only compressed file left but no source file: {}", rec.getCompressedFile());
                        return; // ---
                    }

                    var jobId = logFile.getFileName().toString();
                    jobId = StringUtils.removeEnd(jobId, EXT_LOG).replace("_", "/");

                    var taskId = logFile.getParent().getParent();
                    var fullJobId = FullJobId.parse(taskId.getFileName() + "/" + jobId);
                    log.info("Recompressing and uploading {} -> {}", fullJobId, logFile);

                    var hostname = HostnameUtils.getHostname();
                    var location = getLocation(hostname, fullJobId);
                    compressAndUpload(location);

                    rec.setCompressedFile(location.getLocalCompressedPath());
                    deleteProcessedFile(rec);
                }

                private void deleteProcessedFile(LogRecord rec) {
                    Preconditions.checkState(rec.getLogFile() != null, "logFile cannot be null");
                    Preconditions.checkState(rec.getCompressedFile() != null, "compressedFile cannot be null");
                    try {
                        log.info("Removing old file [{}], last modified is {}",
                                rec.getLogFile(), rec.getLastLogModified());
                        Files.delete(rec.getLogFile());

                        log.info("Removing old compressed file [{}]",
                                rec.getCompressedFile());
                        Files.delete(rec.getCompressedFile());
                    } catch (IOException io) {
                        log.error("Cannot delete old files", io);
                    }
                }

            });
        } catch (IOException e) {
            log.error("Error when processing directory", e);
        }

    }

    public void initBucket() {
        if (!amazonS3.doesBucketExistV2(bucket)) {
            log.info("Registering new Amazon S3 bucket: {}", bucket);
            amazonS3.createBucket(bucket);
        }
    }

    @Override
    public Option<InputStreamSource> getLogFileRead(FullJobId id) {
        return Option.x(getInputStream(id));
    }

    public Optional<InputStreamSource> getInputStream(FullJobId id) {
        return getInputStream(HostnameUtils.getHostname(), id);
    }

    public Optional<InputStreamSource> getInputStream(String hostname, FullJobId id) {
        log.info("Try to get input stream for Job {}", id);
        return getInputStream(getLocation(hostname, id))
                .or(() -> {
                    log.info("Unable to find Job logs: {}", id);
                    return Optional.empty();
                });
    }

    public Optional<InputStreamSource> getInputStreamS3(String hostname, FullJobId id) {
        log.info("Try to get input stream for Job {}", id);
        return getInputStreamS3(getLocation(hostname, id))
                .or(() -> {
                    log.info("Unable to find Job logs: {}", id);
                    return Optional.empty();
                });
    }

    public Optional<InputStreamSource> getInputStream(LogLocation location) {
        return getInputStreamLocal(location)
                .or(() -> getInputStreamS3(location))
                .or(Optional::empty);
    }

    public void uploadFile(LogLocation location) throws IOException {
        var key = location.getS3Key();
        var path = location.getLocalTempCompressedPath();
        log.info("Uploading file into S3 bucket [{}] as [{}]: [{}], file size is {}",
                bucket, key, path, Files.size(path));

        amazonS3.putObject(bucket, key, path.toFile());

        log.info("Uploading file is complete: [{}]", key);
    }

    public LogLocation getLocation(String hostname, FullJobId fullJobId) {
        var taskId = fullJobId.getTaskId();
        var jobId = fullJobId.getJobId();

        var onetimeTask = onetimeTaskChecker != null
                ? onetimeTaskChecker.apply(taskId)
                : Optional.<OnetimeTask>empty();

        var timestamp = Instant.ofEpochMilli(jobId.getTimestamp().getMillis());
        var date = FMT_DATE.format(timestamp);

        var firstHostPart = hostname.indexOf('.');
        var host = firstHostPart > 0
                ? hostname.substring(0, firstHostPart)
                : hostname;
        var jobIdSerialized = jobId.toSerializedString().replace("/", "_");

        var type = onetimeTask.isPresent() ? ONETIME_CLASS : CRON_CLASS;

        var prefixPath = Path.of(type, taskId.toString(), date);

        var relativePath = prefixPath.resolve(jobIdSerialized + EXT_LOG);
        var relativeFinalCompressedPath = relativePath + EXT_LZ4;
        var relativeTempCompressedPath = relativeFinalCompressedPath + EXT_TMP;

        var envPath = Path.of(env).resolve(prefixPath);
        var s3Name = "%s.%s%s%s".formatted(jobIdSerialized, host, EXT_LOG, EXT_LZ4);

        var time = FMT_TIME.format(timestamp);
        var s3Key = envPath.resolve(time).resolve(s3Name).toString();

        return new LogLocation(
                Path.of(workingDir).resolve(relativePath),
                Path.of(workingDir).resolve(relativeTempCompressedPath),
                Path.of(workingDir).resolve(relativeFinalCompressedPath),
                s3Key
        );
    }

    public void compressAndUpload(LogLocation location) throws IOException {
        var path = location.getLocalPath();
        var compressedTmpPath = location.getLocalTempCompressedPath();
        log.info("Compressing [{}] -> [{}]...", path, compressedTmpPath);

        try (var lz4Output = new LZ4FrameOutputStream(Files.newOutputStream(compressedTmpPath))) {
            try (var fileInput = Files.newInputStream(path)) {
                StreamUtils.copy(fileInput, lz4Output);
            }
        }
        uploadFile(location);

        var compressedFinalPath = location.getLocalCompressedPath();
        log.info("Renaming [{}] -> [{}] after successful uploading", compressedTmpPath, compressedFinalPath);
        Files.move(compressedTmpPath, compressedFinalPath);
    }

    private Optional<InputStreamSource> getInputStreamLocal(LogLocation location) {
        var localFile = location.getLocalPath();
        if (Files.exists(localFile)) {
            log.info("Using logs from local file: [{}]", localFile);
            return Optional.of(new FileInputStreamSource(localFile.toFile()));
        } else {
            log.info("No log file found locally: [{}]", localFile);
            return Optional.empty();
        }
    }

    private Optional<InputStreamSource> getInputStreamS3(LogLocation location) {
        return getInputStreamS3(location.getS3Key());
    }

    private Optional<InputStreamSource> getInputStreamS3(String key) {
        log.info("Downloading logs from S3 bucket [{}] as [{}]", bucket, key);

        S3Object object;
        try {
            object = amazonS3.getObject(bucket, key);
        } catch (AmazonS3Exception s3Exception) {
            if (s3Exception.getStatusCode() == 404) { // Not found
                log.info("Unable to find [{}] in S3", key);
                return Optional.empty();
            } else {
                throw s3Exception;
            }
        }
        log.info("File loaded: [{}]", object.getKey());
        var s3Stream = object.getObjectContent().getDelegateStream();
        try {
            var lz4Stream = new LZ4FrameInputStream(s3Stream);
            return Optional.of(new SimpleInputStreamSource(lz4Stream));
        } catch (IOException e) {
            throw new RuntimeException("Unable to parse LZ4 stream from S3: " + key, e);
        }
    }

    static DateTimeFormatter utcFormat(String pattern) {
        return DateTimeFormatter.ofPattern(pattern).withZone(ZoneId.of("UTC"));
    }

    @Value
    public static class LogLocation {
        @Nonnull
        Path localPath;
        @Nonnull
        Path localTempCompressedPath;
        @Nonnull
        Path localCompressedPath;
        @Nonnull
        String s3Key;
    }

    @Data
    private static class LogRecord {
        @Nullable
        Path logFile;
        @Nullable
        Instant lastLogModified;
        @Nullable
        Path compressedFile;
    }
}


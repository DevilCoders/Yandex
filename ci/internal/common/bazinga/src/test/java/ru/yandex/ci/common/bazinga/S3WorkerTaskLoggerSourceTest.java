package ru.yandex.ci.common.bazinga;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.attribute.FileTime;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.TreeSet;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.AmazonS3Exception;
import com.amazonaws.services.s3.model.S3Object;
import com.google.common.util.concurrent.MoreExecutors;
import net.jpountz.lz4.LZ4FrameInputStream;
import org.apache.commons.io.FileUtils;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.junit.jupiter.api.io.TempDir;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.util.StreamUtils;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.util.HostnameUtils;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.impl.worker.LogDirManager;
import ru.yandex.commune.bazinga.impl.worker.WorkerTask;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskRegistry;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.misc.io.SimpleInputStreamSource;
import ru.yandex.misc.io.file.File2;
import ru.yandex.misc.io.file.FileInputStreamSource;
import ru.yandex.misc.log.reqid.RequestIdStack;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class S3WorkerTaskLoggerSourceTest {

    @Mock
    private AmazonS3 amazonS3;

    @Mock
    private WorkerTaskRegistry registry;

    @SuppressWarnings("NotNullFieldNotInitialized")
    @TempDir
    Path tempDir;

    private String bucket;
    private String hostname;
    private S3LogStorage storage;
    private S3WorkerTaskLoggerSource source;

    @BeforeEach
    void init() {
        hostname = "sas5-7268-1.ncop53afu5ns2xab.sas.yp-c.yandex.net";

        bucket = "ci-flow";
        storage = new S3LogStorage(amazonS3, bucket, "testing", tempDir.toString());
        storage.setOnetimeTaskChecker(taskId -> registry.getOnetimeTaskO(taskId).toOptional());

        source = new S3WorkerTaskLoggerSource(storage, MoreExecutors.newDirectExecutorService(), true);
    }

    @AfterEach
    void done() {
        verifyNoMoreInteractions(amazonS3);
    }

    @Test
    void getLocationOnetime() {
        var jobId = FullJobId.parse("flow/20211026T095440.492Z/-1486819287");

        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.of(mock(OnetimeTask.class)));

        var actual = storage.getLocation(hostname, jobId);
        assertThat(actual.getLocalPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log"));

        assertThat(actual.getLocalTempCompressedPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4.tmp"));
        assertThat(actual.getLocalCompressedPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4"));

        assertThat(actual.getS3Key())
                .isEqualTo("testing/onetime/flow/20211026/09/20211026T095440.492Z_-1486819287.sas5-7268-1.log.lz4");
    }

    @Test
    void getLocationDefault() {
        var jobId = FullJobId.parse("flow/20211026T095440.492Z/-1486819287");

        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.empty());

        var actual = storage.getLocation(hostname, jobId);
        assertThat(actual.getLocalPath())
                .isEqualTo(tempDir.resolve("cron/flow/20211026/20211026T095440.492Z_-1486819287.log"));

        assertThat(actual.getLocalTempCompressedPath())
                .isEqualTo(tempDir.resolve("cron/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4.tmp"));
        assertThat(actual.getLocalCompressedPath())
                .isEqualTo(tempDir.resolve("cron/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4"));

        assertThat(actual.getS3Key())
                .isEqualTo("testing/cron/flow/20211026/09/20211026T095440.492Z_-1486819287.sas5-7268-1.log.lz4");
    }

    @Test
    void readFilesNotFound() {
        var jobId = FullJobId.parse("flow/20211026T095440.336Z/33669997");
        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.empty());

        var notFound = new AmazonS3Exception("Not found");
        notFound.setStatusCode(404);
        when(amazonS3.getObject(eq(bucket), anyString()))
                .thenThrow(notFound);

        var fileRef = storage.getInputStream(jobId);
        assertThat(fileRef).isEmpty();

        var location = storage.getLocation(HostnameUtils.getHostname(), jobId);
        verify(amazonS3).getObject(eq(bucket), eq(location.getS3Key()));
        verifyNoMoreInteractions(amazonS3);
    }

    @Test
    void readFilesOtherError() {
        var jobId = FullJobId.parse("flow/20211026T095440.336Z/33669997");
        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.empty());

        var internal = new AmazonS3Exception("Some internal error");
        when(amazonS3.getObject(eq(bucket), anyString()))
                .thenThrow(internal);

        assertThatThrownBy(() -> storage.getInputStream(jobId))
                .isEqualTo(internal);
    }

    @Test
    void readFilesWithAlt() {
        var jobId = FullJobId.parse("flow/20211026T095440.336Z/33669997");
        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.of(Mockito.mock(OnetimeTask.class)));

        var notFound = new AmazonS3Exception("Not found");
        notFound.setStatusCode(404);
        when(amazonS3.getObject(eq(bucket), anyString()))
                .thenThrow(notFound);

        var fileRef = storage.getInputStream(jobId);
        assertThat(fileRef).isEmpty();

        var location = storage.getLocation(HostnameUtils.getHostname(), jobId);
        verify(amazonS3).getObject(eq(bucket), eq(location.getS3Key()));
        verifyNoMoreInteractions(amazonS3);
    }

    @Test
    void writeThenReadLogFiles() throws IOException {
        var task = mock(WorkerTask.class);
        var jobId = FullJobId.parse("flow/20211026T095440.492Z/-1486819287");
        when(task.getFullJobId()).thenReturn(jobId);

        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.of(mock(OnetimeTask.class)));

        var location = storage.getLocation(HostnameUtils.getHostname(), jobId);
        assertThat(location.getLocalPath())
                .doesNotExist();
        assertThat(location.getLocalTempCompressedPath())
                .doesNotExist();
        assertThat(location.getLocalCompressedPath())
                .doesNotExist();

        assertThat(location.getLocalPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log"));
        assertThat(location.getLocalTempCompressedPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4.tmp"));
        assertThat(location.getLocalCompressedPath())
                .isEqualTo(tempDir.resolve("onetime/flow/20211026/20211026T095440.492Z_-1486819287.log.lz4"));

        var text = "test value";
        var data = new Object() {
            File2 file;
        };

        RequestIdStack.clearAndPush(jobId.toSerializedString());
        source.createWorkerTaskLogger(mock(LogDirManager.class))
                .runTaskWithLogger(task, file -> {
                    try {
                        FileUtils.write(file.getFile(), text, StandardCharsets.UTF_8);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    data.file = file;
                });

        assertThat(location.getLocalPath())
                .exists();
        assertThat(location.getLocalTempCompressedPath())
                .doesNotExist();
        assertThat(location.getLocalCompressedPath())
                .exists();

        assertThat(data.file)
                .extracting(File2::getPath)
                .isEqualTo(location.getLocalPath().toString());


        verify(amazonS3).putObject(
                eq(bucket),
                eq(location.getS3Key()),
                any(File.class)
        );

        // Default file
        assertThat(readTextFile(location.getLocalPath())).isEqualTo(text);

        // LZ4 tools compatible format
        assertThat(readLz4File(location.getLocalCompressedPath())).isEqualTo(text);

        assertThat(location.getLocalTempCompressedPath()).doesNotExist();

        // Loaded from default file
        var fileRef = storage.getInputStream(jobId);
        assertThat(fileRef).isNotEmpty();
        assertThat(fileRef.get()).isOfAnyClassIn(FileInputStreamSource.class);

        try (var input = fileRef.get().getInput()) {
            var actualText = StreamUtils.copyToString(input, StandardCharsets.UTF_8);
            assertThat(actualText).isEqualTo(text);
        }

        var s3Object = new S3Object();
        s3Object.setKey(location.getS3Key());
        try (var input = Files.newInputStream(location.getLocalCompressedPath())) {
            var lz4Bytes = StreamUtils.copyToByteArray(input);
            s3Object.setObjectContent(new ByteArrayInputStream(lz4Bytes));
        }
        when(amazonS3.getObject(eq(bucket), eq(location.getS3Key())))
                .thenReturn(s3Object);

        // Loaded from S3
        Files.delete(location.getLocalPath());

        var s3Ref = storage.getLogFileRead(jobId);
        assertThat(s3Ref.isNotEmpty()).isTrue();
        assertThat(s3Ref.get()).isOfAnyClassIn(SimpleInputStreamSource.class);
        try (var input = s3Ref.get().getInput()) {
            var actualText = StreamUtils.copyToString(input, StandardCharsets.UTF_8);
            assertThat(actualText).isEqualTo(text);
        }
    }

    @Test
    void writeWithError() throws IOException {
        var task = mock(WorkerTask.class);
        var jobId = FullJobId.parse("flow/20211026T095440.492Z/-1486819287");
        when(task.getFullJobId()).thenReturn(jobId);

        var text = "test value with error in S3";
        when(registry.getOnetimeTaskO(eq(jobId.getTaskId())))
                .thenReturn(Option.of(mock(OnetimeTask.class)));

        var location = storage.getLocation(HostnameUtils.getHostname(), jobId);

        var internal = new AmazonS3Exception("Some internal error");
        when(amazonS3.putObject(eq(bucket), anyString(), any(File.class)))
                .thenThrow(internal);

        RequestIdStack.clearAndPush(jobId.toSerializedString());
        source.createWorkerTaskLogger(mock(LogDirManager.class))
                .runTaskWithLogger(task, file -> {
                    try {
                        FileUtils.write(file.getFile(), text, StandardCharsets.UTF_8);
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                });

        assertThat(location.getLocalPath())
                .exists();
        assertThat(location.getLocalTempCompressedPath())
                .exists();
        assertThat(location.getLocalCompressedPath())
                .doesNotExist();

        // LZ4 tools compatible format
        assertThat(readLz4File(location.getLocalTempCompressedPath())).isEqualTo(text);

    }

    @Test
    void writeThenCleanupMultipleFiles() throws IOException, InterruptedException {
        var fmt = S3LogStorage.utcFormat("yyyyMMddHHmmss");

        when(registry.getOnetimeTaskO(eq(new TaskId("flow"))))
                .thenReturn(Option.of(mock(OnetimeTask.class)));

        var allFiles = Collections.synchronizedList(new ArrayList<String>());

        var dates = List.of("20211025", "20211026", "20211027", "20211028", "20211029");
        var executor = Executors.newFixedThreadPool(dates.size());

        // Make sure all logs are written independently
        dates.forEach(date ->
                executor.execute(() -> {
                    try {
                        var task = mock(WorkerTask.class);
                        var id = "%sT111111.492Z/-1486819287".formatted(date);
                        var jobId = FullJobId.parse("flow/" + id);
                        when(task.getFullJobId()).thenReturn(jobId);

                        RequestIdStack.clearAndPush(jobId.toSerializedString());
                        source.createWorkerTaskLogger(mock(LogDirManager.class))
                                .runTaskWithLogger(task, file -> {
                                    try {
                                        try (var writer = Files.newBufferedWriter(file.getFile().toPath())) {
                                            writer.write("value-" + date);
                                            Thread.sleep(500);
                                            writer.write("\n");
                                        }
                                    } catch (Exception e) {
                                        throw new RuntimeException(e);
                                    }
                                });

                        var location = storage.getLocation(HostnameUtils.getHostname(), jobId);
                        allFiles.add(location.getLocalPath().toString());
                        allFiles.add(location.getLocalCompressedPath().toString());

                        assertThat(location.getLocalPath()).exists();
                        assertThat(location.getLocalCompressedPath()).exists();

                        var dateObj = Instant.from(fmt.parse(date + "111111"));
                        Files.setLastModifiedTime(location.getLocalPath(), FileTime.from(dateObj));
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                })
        );
        executor.shutdown();
        //noinspection ResultOfMethodCallIgnored
        executor.awaitTermination(15, TimeUnit.SECONDS);

        reset(amazonS3);
        Collections.sort(allFiles);

        for (int i = 0; i < dates.size(); i++) {
            assertThat(readTextFile(Path.of(allFiles.get(i * 2))))
                    .isEqualTo("value-" + dates.get(i) + "\n")
                    .describedAs("Checking files at %s", i);
        }

        var now = Instant.from(fmt.parse("20211029111112"));

        // Nothing to clean
        storage.cleanup(Duration.ofDays(14), now);
        assertThat(tempDir.resolve("onetime/flow/20211025")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211026")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211027")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211028")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211029")).exists();
        assertThat(listFiles())
                .isEqualTo(allFiles);

        // Clean first files
        storage.cleanup(Duration.ofDays(4), now);
        assertThat(tempDir.resolve("onetime/flow/20211025")).doesNotExist();
        assertThat(tempDir.resolve("onetime/flow/20211026")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211027")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211028")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211029")).exists();
        allFiles.remove(0);
        allFiles.remove(0);
        assertThat(listFiles())
                .isEqualTo(allFiles);

        // Clean all but last 2 days
        storage.cleanup(Duration.ofDays(2), now);
        assertThat(tempDir.resolve("onetime/flow/20211026")).doesNotExist();
        assertThat(tempDir.resolve("onetime/flow/20211027")).doesNotExist();
        assertThat(tempDir.resolve("onetime/flow/20211028")).exists();
        assertThat(tempDir.resolve("onetime/flow/20211029")).exists();
        allFiles.remove(0);
        allFiles.remove(0);
        allFiles.remove(0);
        allFiles.remove(0);
        assertThat(listFiles())
                .isEqualTo(allFiles);

        var lz4File = allFiles.get(1);
        assertThat(lz4File)
                .endsWith("/20211028T111111.492Z_-1486819287.log.lz4");
        Files.delete(tempDir.resolve("onetime/flow/20211028").resolve(lz4File));

        // Clean all, keep last day
        storage.cleanup(Duration.ofMillis(0), now);
        assertThat(tempDir.resolve("onetime/flow/20211028")).doesNotExist(); // No lz4 file - recompressing
        assertThat(tempDir.resolve("onetime/flow/20211029")).exists();
        allFiles.clear();
        assertThat(listFiles())
                .isEqualTo(allFiles);


        var jobId = FullJobId.parse("flow/20211028T111111.492Z/-1486819287"); // recompressed
        var actual = storage.getLocation(HostnameUtils.getHostname(), jobId);
        verify(amazonS3).putObject(
                eq(bucket),
                eq(actual.getS3Key()),
                any(File.class)
        );
    }

    private String readTextFile(Path textFile) throws IOException {
        try (var input = Files.newInputStream(textFile)) {
            return StreamUtils.copyToString(input, StandardCharsets.UTF_8);
        }
    }

    private String readLz4File(Path lz4File) throws IOException {
        try (var input = new LZ4FrameInputStream(Files.newInputStream(lz4File))) {
            return StreamUtils.copyToString(input, StandardCharsets.UTF_8);
        }
    }

    private List<String> listFiles() throws IOException {
        var files = new TreeSet<String>();
        Files.walkFileTree(tempDir, new SimpleFileVisitor<>() {
            @Override
            public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) throws IOException {
                files.add(file.toString());
                return super.visitFile(file, attrs);
            }
        });
        return List.copyOf(files);
    }

}

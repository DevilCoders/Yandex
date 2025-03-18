package ru.yandex.ci.engine.pciexpress;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import org.eclipse.jetty.io.RuntimeIOException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.sandbox.ProxySandboxClient;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.definition.context.impl.JobContextImpl;
import ru.yandex.ci.tasklet.SandboxResource;
import ru.yandex.repo.pciexpress.proto.Dto;

import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;


class GetPciDssCommitResultJobTest extends EngineTestBase {

    @Test
    void execute() throws Exception {
        JobContextImpl context = mock(JobContextImpl.class);
        var resources = mock(JobResourcesContext.class);

        var resource = SandboxResource.newBuilder()
                .setType("PCI_EXPRESS_JOG")
                .setId(1234567890)
                .build();

        when(resources.consumeList(SandboxResource.class)).thenReturn(List.of(resource));
        when(context.resources()).thenReturn(resources);
        doReturn(new InputStreamResource(
                new ByteArrayInputStream(
                        Dto.LegsOutput.newBuilder()
                                .setCommitId("some-commit-id")
                                .addResults(
                                        Dto.BuildResult.newBuilder()
                                                .setError(
                                                        Dto.BuildError.newBuilder()
                                                                .setText("Error happened")
                                                                .setRetryable(true).build()
                                                )
                                                .setPackage(
                                                        Dto.PackageInfo.newBuilder()
                                                                .setPackage("path/to/package.json")
                                                                .build()
                                                )
                                                .build())
                                .build().toByteArray()
                )
        )).when(proxySandboxClient).downloadResource(1234567890);

        GetPciDssCommitResultJob job = new GetPciDssCommitResultJob(proxySandboxClient);
        job.execute(context);

    }

    @RequiredArgsConstructor
    private static class InputStreamResource implements ProxySandboxClient.CloseableResource {

        @Nonnull
        private final InputStream inputStream;

        @Override
        public InputStream getStream() {
            return inputStream;
        }

        @Override
        public void close() {
            try {
                inputStream.close();
            } catch (IOException e) {
                throw new RuntimeIOException(e);
            }
        }
    }
}

package ru.yandex.ci.client.sandbox;

import java.io.Closeable;
import java.io.InputStream;

import ru.yandex.lang.NonNullApi;

@NonNullApi
public interface ProxySandboxClient {
    CloseableResource downloadResource(long resourceId);

    /**
     * Must call {@link #close()} upon complete processing
     */
    interface CloseableResource extends Closeable {
        InputStream getStream();

        @Override
        void close();
    }
}

package ru.yandex.ci.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.net.URL;
import java.nio.charset.StandardCharsets;

import javax.annotation.Nonnull;

import com.github.luben.zstd.ZstdInputStream;
import com.google.common.io.ByteSource;
import com.google.common.io.Resources;

public class ResourceUtils {

    private ResourceUtils() {
        //
    }

    public static URL url(String resource) {
        return Resources.getResource(resource);
    }

    public static String textResource(String resource) {
        try {
            return asSource(resource).asCharSource(StandardCharsets.UTF_8).read();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public static byte[] binaryResource(String resource) {
        try {
            return asSource(resource).read();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    public static InputStream stream(String resource) {
        try {
            return asSource(resource).openStream();
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private static ByteSource asSource(String resource) {
        return wrap(Resources.asByteSource(url(resource)), resource);
    }

    private static ByteSource wrap(ByteSource source, String resource) {
        if (resource.endsWith(".zst")) {
            return new ZstdByteSource(source);
        } else {
            return source;
        }
    }

    private static class ZstdByteSource extends ByteSource {

        private final ByteSource source;

        private ZstdByteSource(ByteSource source) {
            this.source = source;
        }

        @Nonnull
        @Override
        public InputStream openStream() throws IOException {
            return new ZstdInputStream(source.openStream());
        }
    }
}

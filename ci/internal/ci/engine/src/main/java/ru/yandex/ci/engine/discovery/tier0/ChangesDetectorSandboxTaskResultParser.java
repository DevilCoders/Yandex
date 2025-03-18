package ru.yandex.ci.engine.discovery.tier0;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.util.zip.GZIPInputStream;

import com.fasterxml.jackson.core.JsonFactory;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import com.google.common.io.ByteStreams;
import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;

/**
 * Class parses result of ChangesDetector sandbox task: https://nda.ya.ru/t/ksaPnaVR3nGmZh
 */
public class ChangesDetectorSandboxTaskResultParser {

    private static final JsonFactory JSON_FACTORY = new JsonFactory();
    private final AffectedPathMatcher affectedPathMatcher;

    ChangesDetectorSandboxTaskResultParser(AffectedPathMatcher affectedPathMatcher) {
        this.affectedPathMatcher = affectedPathMatcher;
    }

    void parse(String sourceFileName, byte[] source) {
        var in = new ByteArrayInputStream(source);
        try (var jsonStream = decompressAndUnTar(sourceFileName, in)) {
            parseJson(jsonStream, affectedPathMatcher);
        } catch (IOException e) {
            throw new GraphDiscoveryException(e);
        }
    }

    void parse(String sourceFileName, InputStream source) {
        try (var jsonStream = decompressAndUnTar(sourceFileName, source)) {
            parseJson(jsonStream, affectedPathMatcher);
        } catch (IOException e) {
            throw new GraphDiscoveryException(e);
        }
    }

    public static InputStream decompressAndUnTar(String sourceFileName, InputStream inputStream) {
        var extension = getFileExtension(sourceFileName);
        try {
            switch (extension) {
                case "gz":
                    return decompressAndUnTar(
                            getFileNameWithoutExtension(sourceFileName),
                            new GZIPInputStream(inputStream)
                    );
                case "tar":
                    var tarIn = new TarArchiveInputStream(inputStream);
                    // we expect only one file entry
                    var entry = (TarArchiveEntry) tarIn.getNextEntry();
                    checkOrThrow(entry != null, "No found entry inside tar");
                    checkOrThrow(entry.isFile(), "No file found inside tar");
                    return decompressAndUnTar(
                            getFileNameWithoutExtension(sourceFileName),
                            ByteStreams.limit(tarIn, entry.getSize())
                    );
                default:
                    return inputStream;
            }
        } catch (IOException e) {
            throw new GraphDiscoveryException(e);
        }
    }

    static void parseJson(InputStream jsonStream, AffectedPathMatcher affectedPathMatcher) {
        try (var parser = JSON_FACTORY.createParser(jsonStream)) {
            parser.nextToken();
            checkOrThrow(
                    parser.currentToken() == JsonToken.START_OBJECT,
                    parser, "Expected array %s, but found %s", JsonToken.START_OBJECT, parser.currentToken()
            );

            while (parser.nextToken() != JsonToken.END_OBJECT) {
                var platform = GraphDiscoveryHelper.sandboxPlatformParamToPlatform(parser.getCurrentName());

                parser.nextToken();
                checkOrThrow(
                        parser.currentToken() == JsonToken.START_ARRAY,
                        parser, "Expected array %s, but found %s", JsonToken.START_ARRAY, parser.currentToken()
                );

                while (parser.nextToken() != JsonToken.END_ARRAY) {
                    affectedPathMatcher.accept(platform, Path.of(parser.getText()));
                }
            }
        } catch (IOException e) {
            throw new GraphDiscoveryException(e);
        }
    }

    private static String getFileExtension(String fileName) {
        int dotIndex = fileName.lastIndexOf('.');
        return (dotIndex == -1) ? "" : fileName.substring(dotIndex + 1);
    }

    private static String getFileNameWithoutExtension(String fileName) {
        int dotIndex = fileName.lastIndexOf('.');
        return (dotIndex == -1) ? fileName : fileName.substring(0, dotIndex);
    }

    private static void checkOrThrow(boolean condition, JsonParser parser, String messageTemplate, Object... args) {
        if (!condition) {
            throw new GraphDiscoveryException(
                    new JsonParseException(parser, messageTemplate.formatted(args))
            );
        }
    }

    private static void checkOrThrow(boolean condition, String messageTemplate, Object... args) {
        if (!condition) {
            throw new GraphDiscoveryException(messageTemplate.formatted(args));
        }
    }

}

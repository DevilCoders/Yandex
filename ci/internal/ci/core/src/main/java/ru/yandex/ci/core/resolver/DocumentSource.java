package ru.yandex.ci.core.resolver;

import java.util.function.Supplier;

import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public interface DocumentSource {
    Logger log = LoggerFactory.getLogger(DocumentSource.class);

    /**
     * Prefix to make full resolution
     *
     * @return custom prefix for JmesPath, additional resolution is required when mentioned in JmesPath.
     * This feature can be used to skip upstream resources loading, but it is required to
     * explicitly define resolution prefix: "${tasks.*.a}" is OK, but "${*.*.a}" is not OK
     */
    default String getResolutionPrefix() {
        return PropertiesSubstitutor.TASKS_KEY;
    }

    /**
     * Prefix to skip expression evaluation
     *
     * @return prefix, if provided than expression will be skipped during resolution
     */
    default String getSkipPrefix() {
        return "";
    }

    /**
     * Get document
     *
     * @param fullResolution true to provide full document resolution (include all tasks, for instance)
     * @return complete document
     */
    JsonObject getDocument(boolean fullResolution);

    //

    // No additional resolution required
    static DocumentSource of(JsonObject document) {
        return of(() -> document);
    }

    // No additional resolution required
    static DocumentSource of(Supplier<JsonObject> documentSource) {
        return of(documentSource, documentSource);
    }

    static DocumentSource of(Supplier<JsonObject> basicSupplier, Supplier<JsonObject> fullSupplier) {
        return new CachedDocumentSource(basicSupplier, fullSupplier);
    }

    static DocumentSource skipContext(DocumentSource delegate) {
        return new DocumentSource() {
            @Override
            public String getResolutionPrefix() {
                return delegate.getResolutionPrefix();
            }

            @Override
            public String getSkipPrefix() {
                return PropertiesSubstitutor.CONTEXT_KEY;
            }

            @Override
            public JsonObject getDocument(boolean fullResolution) {
                return delegate.getDocument(fullResolution);
            }
        };
    }

    static DocumentSource merge(DocumentSource delegate, JsonObject mergeWith) {
        return new CachedDocumentSource(
                () -> {
                    var merge = delegate.getDocument(false);
                    mergeWith.entrySet().forEach(e -> merge.add(e.getKey(), e.getValue()));
                    return merge;
                },
                () -> delegate.getDocument(true)
        ) {
            @Override
            public String getResolutionPrefix() {
                return delegate.getResolutionPrefix();
            }

            @Override
            public String getSkipPrefix() {
                return delegate.getSkipPrefix();
            }
        };
    }


    @RequiredArgsConstructor
    class CachedDocumentSource implements DocumentSource {
        private final Supplier<JsonObject> basicSupplier;
        private final Supplier<JsonObject> fullSupplier;

        @Nullable
        // Do not store context into Gson if there is no lookup operation for context
        private JsonObject basic;

        @Nullable
        // Do not store tasks into Gson if there is no lookup operations for tasks
        private JsonObject full;

        @Override
        public JsonObject getDocument(boolean fullResolution) {
            if (basic == null) {
                basic = basicSupplier.get();
                if (basic.size() > 0) {
                    if (basicSupplier == fullSupplier) {
                        log.info("JMESSource: {}", basic);
                    } else {
                        log.info("JMES Basic Source: {}", basic);
                    }
                }
            }
            if (fullResolution) {
                if (basicSupplier == fullSupplier) {
                    return basic;
                }
                if (full == null) {
                    full = fullSupplier.get();
                    basic.entrySet().forEach(e -> full.add(e.getKey(), e.getValue()));
                    if (full.size() > 0) {
                        log.info("JMES Full Source: {}", full);
                    }
                }
                return full;
            } else {
                return basic;
            }
        }
    }
}

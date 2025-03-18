package ru.yandex.ci.core.resolver;

import java.util.List;
import java.util.Objects;
import java.util.function.Function;
import java.util.regex.Pattern;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Strings;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import io.burt.jmespath.RuntimeConfiguration;
import io.burt.jmespath.function.FunctionRegistry;
import io.burt.jmespath.gson.GsonRuntime;
import lombok.RequiredArgsConstructor;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.resolver.function.AddFunc;
import ru.yandex.ci.core.resolver.function.CaseFunction;
import ru.yandex.ci.core.resolver.function.DateFormatFunc;
import ru.yandex.ci.core.resolver.function.JoinAnyFunc;
import ru.yandex.ci.core.resolver.function.NotEmptyFunc;
import ru.yandex.ci.core.resolver.function.RangeFunction;
import ru.yandex.ci.core.resolver.function.ReplaceFunc;
import ru.yandex.ci.core.resolver.function.RootFunc;
import ru.yandex.ci.core.resolver.function.SingleFunc;
import ru.yandex.ci.core.resolver.function.ToJsonFunc;

/**
 * <p>
 * Resolve variables using JMESPath with all known types support: String, Number, Boolean, Character.
 * </p>
 *
 *
 * <p>Support 2 modes:</p>
 * <ul>
 *     <li>As "${...}" expression - should substitute value entirely, supporting String, Number, Boolean, Character,
 *     JsonObject and JsonArray; i.e. replace expression with JMESPath search result</li>
 *     <li>As part of "abc ${...} cde" expression - in this case no JsonObject nor JsonArray are allowed in
 *     JMESPath; this restricted by choice, could be changed latet</li>
 * </ul>
 */
public class JmesPathResolver {

    private static final ThreadLocal<JsonObject> ROOT_OBJECT = new ThreadLocal<>();

    private final GsonRuntime runtimeReplace;
    private final GsonRuntime runtimeCleanup;

    JmesPathResolver() {
        var runtimeConfiguration = new RuntimeConfiguration.Builder()
                .withFunctionRegistry(FunctionRegistry.defaultRegistry()
                        .extend(
                                new JoinAnyFunc(),
                                new SingleFunc(),
                                new NotEmptyFunc(),
                                new AddFunc(),
                                new RootFunc(ROOT_OBJECT::get),
                                new ToJsonFunc(),
                                new ReplaceFunc(),
                                new RangeFunction(),
                                new CaseFunction("lowercase", String::toLowerCase),
                                new CaseFunction("uppercase", String::toUpperCase),
                                new CaseFunction("capitalize", StringUtils::capitalize),
                                new DateFormatFunc()
                        )
                );
        this.runtimeReplace = new GsonRuntime(runtimeConfiguration.build());
        this.runtimeCleanup = new GsonRuntime(runtimeConfiguration.withSilentTypeErrors(true).build());
    }

    public boolean hasSubstitution(@Nonnull String origExpression) {
        return hasPrefix(origExpression);
    }

    public Function<String, JsonElement> lookup(DocumentSource documentSource, boolean cleanup) {
        return new SingleLookup(documentSource, cleanup);
    }

    private String trim(String expression) {
        return expression.trim();
    }

    private static boolean hasPrefix(String expression) {
        return expression.contains(JmesPathExtractor.FULL_PREFIX);
    }

    //

    @RequiredArgsConstructor
    private class SingleLookup implements Function<String, JsonElement> {
        private final DocumentSource documentSource;
        private final boolean cleanup;

        @Nullable
        private Patterns patterns;

        @Override
        public JsonElement apply(String origExpression) {
            Objects.requireNonNull(origExpression, "origExpression cannot be null");

            if (!hasPrefix(origExpression)) {
                return new JsonPrimitive(origExpression);
            }

            var expression = origExpression.trim();
            if (expression.isEmpty()) {
                return new JsonPrimitive(origExpression);
            }

            var parts = JmesPathExtractor.extract(expression);

            // String "${a.b.c}" is fully replaceable
            if (parts.size() == 1) {
                return resolveSingle(expression, parts.get(0));
            } else {
                return resolveMultiple(expression, parts);
            }
        }

        // Mode 1: full replacement
        private JsonElement resolveSingle(String expression, JmesPathExtractor.Part part) {
            if (part.isReplace()) {
                var singleExpression = part.getText();

                var unwrappedExpression = trim(singleExpression);
                if (isSkip(unwrappedExpression)) {
                    return new JsonPrimitive(expression);
                }
                var result = searchImpl(unwrappedExpression, documentSource);
                if (result.isJsonNull()) {
                    if (!cleanup) {
                        throw new IllegalArgumentException("No value resolved for expression: " +
                                unwrappedExpression);
                    }
                }
                return result;
            } else {
                // could be "$${a.b.c}"
                return new JsonPrimitive(part.getText());
            }
        }

        // Mode 2: partial replacement
        private JsonElement resolveMultiple(String expression, List<JmesPathExtractor.Part> parts) {

            Function<String, String> resolver = key -> {
                var unwrappedExpression = trim(key);
                if (isSkip(unwrappedExpression)) {
                    return null;
                }
                var result = searchImpl(unwrappedExpression, documentSource);
                if (result.isJsonPrimitive()) {
                    return result.getAsString();
                } else if (result.isJsonNull()) {
                    if (cleanup) {
                        return null;
                    } else {
                        throw new IllegalArgumentException("No value resolved for expression: " +
                                unwrappedExpression);
                    }
                } else {
                    throw new IllegalArgumentException(String.format(
                            "Invalid expression: %s, must return simple object " +
                                    "(string, number, boolean or character), got %s",
                            unwrappedExpression, result.getClass()));
                }
            };

            var buffer = new StringBuilder(expression.length());

            for (var part : parts) {
                var key = part.getText();
                if (part.isReplace()) {
                    var value = resolver.apply(key);
                    if (value == null) {
                        buffer.append(JmesPathExtractor.FULL_PREFIX).append(key).append(JmesPathExtractor.SUFFIX);
                    } else {
                        buffer.append(value);
                    }
                } else {
                    buffer.append(key);
                }
            }
            return new JsonPrimitive(buffer.toString());
        }

        private JsonElement searchImpl(String expression, DocumentSource documentSource) {
            var resolutionPattern = getPatterns().resolutionPattern;
            var needResolution = resolutionPattern != null && resolutionPattern.matcher(expression).find();
            var document = documentSource.getDocument(needResolution);
            var runtime = cleanup ? runtimeCleanup : runtimeReplace;

            ROOT_OBJECT.set(document);
            try {
                return runtime.compile(expression).search(document);
            } finally {
                ROOT_OBJECT.remove();
            }
        }

        private boolean isSkip(String expression) {
            var skipPrefix = getPatterns().skipPrefix;
            return skipPrefix != null && skipPrefix.matcher(expression).find();
        }

        private Patterns getPatterns() {
            if (patterns == null) {
                patterns = new Patterns(documentSource);
            }
            return patterns;
        }
    }


    private static class Patterns {
        @Nullable
        private final Pattern resolutionPattern;
        @Nullable
        private final Pattern skipPrefix;

        private Patterns(DocumentSource documentSource) {
            this.resolutionPattern = getResolutionPattern(documentSource.getResolutionPrefix());
            this.skipPrefix = ofSkipPrefix(documentSource.getSkipPrefix());
        }

        @Nullable
        private static Pattern getResolutionPattern(String prefix) {
            if (Strings.isNullOrEmpty(prefix)) {
                return null;
            } else {
                return Pattern.compile("\\b%s\\b".formatted(prefix));
            }
        }

        @Nullable
        private static Pattern ofSkipPrefix(String prefix) {
            if (Strings.isNullOrEmpty(prefix)) {
                return null;
            } else {
                return Pattern.compile("^\\b%s\\b".formatted(prefix));
            }
        }
    }
}

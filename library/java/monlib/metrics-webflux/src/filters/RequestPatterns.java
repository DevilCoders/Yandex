package ru.yandex.monlib.metrics.webflux.filters;

import java.util.Optional;
import java.util.stream.Collectors;

import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.stereotype.Component;
import org.springframework.web.reactive.result.method.annotation.RequestMappingHandlerMapping;
import org.springframework.web.util.pattern.PathPattern;

/**
 * @author Sergey Polovko
 */
@Component
public class RequestPatterns {

    private final RequestPatternResolver resolver;

    public RequestPatterns(RequestMappingHandlerMapping handlerMapping) {
        var endpoints = handlerMapping.getHandlerMethods()
                .keySet()
                .stream()
                .flatMap(r -> r.getPatternsCondition().getPatterns().stream())
                .map(PathPattern::toString)
                .collect(Collectors.toSet());
        this.resolver = new RequestPatternResolver(endpoints);
    }

    public Optional<String> findRequestPattern(ServerHttpRequest request) {
        return resolver.resolve(request.getPath().toString())
                .map(RequestPatterns::fixPathPattern);
    }

    /**
     * Some path pattern symbols isn't supported in Solomon labels, e.g. "{" and "}",
     * So we fix path pattern:
     * before - /api/v2/projects/{projectId}/,
     * after - /api/v2/projects/:projectId
     *
     * @see ru.yandex.monlib.metrics.labels.validate.StrictValidator
     **/
    private static String fixPathPattern(String pattern) {
        if (pattern.endsWith("/")) {
            pattern = pattern.substring(0, pattern.length() - 1);
        }
        return pattern.replaceAll("\\{", ":").replaceAll("}", "").replaceAll("[?*]", "_");
    }
}

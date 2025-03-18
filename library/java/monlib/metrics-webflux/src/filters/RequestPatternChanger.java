package ru.yandex.monlib.metrics.webflux.filters;

import org.springframework.http.server.reactive.ServerHttpRequest;

/**
 * @author Alexey Trushkin
 */
public interface RequestPatternChanger {

    /**
     * Fixes result pattern as needed
     *
     * @param request current http request
     * @param pattern pattern to change
     * @return changed pattern
     */
    String change(ServerHttpRequest request, String pattern);
}

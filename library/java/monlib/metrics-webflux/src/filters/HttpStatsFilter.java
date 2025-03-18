package ru.yandex.monlib.metrics.webflux.filters;

import java.net.URI;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import javax.annotation.ParametersAreNonnullByDefault;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.Import;
import org.springframework.http.HttpStatus;
import org.springframework.http.server.reactive.ServerHttpRequest;
import org.springframework.stereotype.Component;
import org.springframework.web.server.ServerWebExchange;
import org.springframework.web.server.WebFilter;
import org.springframework.web.server.WebFilterChain;
import reactor.core.publisher.Mono;

import ru.yandex.monlib.metrics.http.HttpStatsMetrics;
import ru.yandex.monlib.metrics.registry.MetricRegistry;

/**
 * @author Oleg Baryshnikov
 */
@Component
@Import(RequestPatterns.class)
@ParametersAreNonnullByDefault
public class HttpStatsFilter implements WebFilter {
    private static final Logger logger = LoggerFactory.getLogger(HttpStatsFilter.class);

    private final HttpStatsMetrics httpStatsMetrics;
    private final RequestPatterns patterns;
    private final List<RequestPatternChanger> requestPatternChangers;

    public HttpStatsFilter(MetricRegistry metricRegistry, RequestPatterns patterns, List<RequestPatternChanger> requestPatternChangers) {
        this.httpStatsMetrics = new HttpStatsMetrics(metricRegistry);
        this.patterns = patterns;
        this.requestPatternChangers = requestPatternChangers;
    }

    @Override
    public Mono<Void> filter(ServerWebExchange exchange, WebFilterChain chain) {
        long startNanos = System.nanoTime();
        var endpointMetrics = findEndpointMetrics(exchange.getRequest());
        endpointMetrics.callStarted();

        return chain.filter(exchange)
                .doFinally(signalType -> {
                    long durationMillis = TimeUnit.NANOSECONDS.toMillis(System.nanoTime() - startNanos);
                    HttpStatus statusCode = exchange.getResponse().getStatusCode();
                    endpointMetrics.callCompleted(statusCode == null ? 0 : statusCode.value(), durationMillis);
                });
    }

    private HttpStatsMetrics.EndpointMetrics findEndpointMetrics(ServerHttpRequest request) {
        Optional<String> patternOpt = patterns.findRequestPattern(request);

        if (patternOpt.isPresent()) {
            String pattern = patternOpt.get();

            for (RequestPatternChanger requestPatternChanger : requestPatternChangers) {
                pattern = requestPatternChanger.change(request, pattern);
            }

            return httpStatsMetrics.getEndpointsMetrics(pattern, request.getMethodValue());
        }

        logger.info("cannot resolve pattern for request: " + requestUri(request));
        return httpStatsMetrics.getEndpointsMetrics("unknown", request.getMethodValue());
    }

    public static String requestUri(ServerHttpRequest req) {
        URI uri = req.getURI();
        String rawQuery = uri.getRawQuery();
        return (rawQuery == null) ? uri.getPath() : uri.getPath() + '?' + rawQuery;
    }
}

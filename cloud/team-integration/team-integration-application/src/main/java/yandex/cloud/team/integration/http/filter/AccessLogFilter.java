package yandex.cloud.team.integration.http.filter;

import java.util.List;

import javax.servlet.annotation.WebFilter;

import yandex.cloud.common.httpserver.filter.AbstractJettyAccessLogFilter;
import yandex.cloud.common.httpserver.log.TvmTicketRule;
import yandex.cloud.log.DefaultJsonSanitizer;
import yandex.cloud.log.JsonSanitizer;
import yandex.cloud.team.integration.application.Application;

@WebFilter(filterName = "accessLogFilter", urlPatterns = "/*", asyncSupported = true)
public final class AccessLogFilter extends AbstractJettyAccessLogFilter {

    private static final int MAX_HEADER_LENGTH = 128;

    public AccessLogFilter() {
        super(Application.NAME, LogLevel.BODY, createSanitizer(), MAX_HEADER_LENGTH);
    }

    private static JsonSanitizer createSanitizer() {
        return new DefaultJsonSanitizer(List.of(new TvmTicketRule()));
    }

}

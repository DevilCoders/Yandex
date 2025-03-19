package yandex.cloud.team.integration.http.filter;

import javax.servlet.annotation.WebFilter;

import yandex.cloud.common.httpserver.filter.AbstractMetricsFilter;
import yandex.cloud.team.integration.application.Application;

@WebFilter(filterName = "metricsFilter", urlPatterns = "/*")
public final class MetricsFilter extends AbstractMetricsFilter {

    public MetricsFilter() {
        super(Application.NAME);
    }

}

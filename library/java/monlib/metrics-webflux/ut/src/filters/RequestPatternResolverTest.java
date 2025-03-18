package ru.yandex.monlib.metrics.webflux.filters;

import java.util.Optional;
import java.util.Set;

import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

/**
 * @author Vladimir Gordiychuk
 */
public class RequestPatternResolverTest {

    private RequestPatternResolver resolver;

    @Before
    public void setUp() {
        Set<String> endpoints = Set.of(
            "/monitoring/v1/metrics/labelValues",
            "/monitoring/v1/channels",
            "/push",
            "/api/internal/projects/{projectId}/shardLabels",
            "/swagger-resources/configuration/ui",
            "/monitoring/v1/data/write",
            "/api/v2/projects/{projectId}/sensors/names",
            "/api/v2/projects/{projectId}/alerts/{alertId}/state/evaluationStats",
            "/monitoring/v1/alerts",
            "/api/v2/projects/{projectId}/dashboards",
            "/staffOnly",
            "/api/internal/settings",
            "/api/v2/projects/{projectId}/clusters",
            "/api/internal/projects/{projectId}/dashboards/{dashboardId}",
            "/api/v2/projects/{projectId}/services/{serviceId}/clusters",
            "/api/internal/projects/{projectId}/matchingShard",
            "/api/internal/projects/{projectId}/menu",
            "/swagger-resources/configuration/security",
            "/api/v2/projects/{projectId}/alerts/{alertId}/state/evaluation",
            "/monitoring/v2/metrics/labels/{key}/values",
            "/api/v2/projects/{projectId}/dashboards/{dashboardId}",
            "/api/v2/projects/{projectId}/alerts/{parentId}/subAlerts/{alertId}",
            "/monitoring/v1/channels/status/history",
            "/monitoring/v1/metrics/labelKeys",
            "/api/v2/projects/{projectId}/alerts/explainEvaluation",
            "/monitoring/v1/alerts/state/evaluation",
            "/api/v2/projects/{projectId}/services",
            "/api/internal/pinByUrl",
            "/api/v2/projects",
            "/api/v2/projects/{projectId}/alerts/{alertId}/state/notificationStats",
            "/rest/grafana/search",
            "/monitoring/v1/alerts/evaluation/history",
            "/api/v2/telegram/groupTitles",
            "/api/internal/settings/{key}",
            "/api/v2/projects/{projectId}/services/{serviceId}",
            "/api/v2/push",
            "/api/internal/projects/{projectId}/graphs",
            "/monitoring/v1/data/read",
            "/monitoring/v1/metrics",
            "/api/v2/projects/{projectId}/shards/{shardId}",
            "/api/v2/projects/{projectId}/sensors/labels",
            "/rest/graph-data",
            "/monitoring/v1/expression/functions",
            "/rest/grafana/sensors/value/list",
            "/api/v2/projects/{projectId}/graphs/{graphId}",
            "/api/v2/projects/{projectId}/notificationChannels/{notificationChannelId}",
            "/api/internal/shards",
            "/push/json",
            "/rest/grafana/sensors/key/list",
            "/api/v2/projects/{projectId}/notificationChannels",
            "/api/v3alpha/graphs",
            "/api/v2/projects/{projectId}/alerts/{alertId}",
            "/data-api/get",
            "/api/v2/projects/{projectId}/clusters/{clusterId}/services",
            "/api/v2/projects/{projectId}/alerts/subAlerts/explainEvaluation",
            "/data-api/sensors",
            "/api/v2/projects/{projectId}/graphs",
            "/monitoring/v1/metrics/names",
            "/api/v2/projects/{projectId}/alerts/{parentId}/subAlerts/{alertId}/explainEvaluation",
            "/api/v2/projects/{projectId}/alerts/{parentId}/subAlerts/{alertId}/state/notification",
            "/monitoring/v1/alerts/alert",
            "/api/v2/projects/{projectId}/shards/{shardId}/targets",
            "/api/internal/projects/{projectId}/graphs/{graphId}",
            "/api/v2/projects/{projectId}/alerts/{alertId}/explainEvaluation",
            "/api/v2/projects/{projectId}/alerts/{parentId}/subAlerts/{alertId}/state/evaluation",
            "/monitoring/v1/alerts/simulation/alert",
            "/api/v2/projects/{projectId}/clusters/{clusterId}",
            "/monitoring/v2/metrics/names",
            "/api/v2/projects/{projectId}/sensors/data",
            "/api/v2/projects/{projectId}/alerts",
            "/api/v3alpha/dashboards",
            "/monitoring/v2/metrics",
            "/api/internal/fetcher/resolveCluster",
            "/api/internal/projects/{projectId}/shards/{shardId}/reload",
            "/api/v2/projects/{projectId}/sensors/sensorNames",
            "/api/v2/projects/{projectId}/sensors",
            "/api/v2/projects/{projectId}/alerts/{alertId}/state/notification",
            "/staffOnly/{address}/**",
            "/api/v3alpha/dashboard",
            "/api/v2/projects/{id}",
            "/api/internal/services",
            "/api/internal/projects/{projectId}/dashboards",
            "/rest/grafana/sensors/search",
            "/rest/generic",
            "/api/v2/projects/{projectId}/alerts/stats",
            "/rest/selectorValues",
            "/monitoring/v1/alerts/state/notification",
            "/swagger-resources",
            "/api/internal/clusters",
            "/api/internal/oauth",
            "/monitoring/v1/channels/channel",
            "/api/internal/auth",
            "/monitoring/v2/data/read",
            "/monitoring/v2/metrics/labels",
            "/api/internal/roles",
            "/api/internal/projects/{projectId}/stockpile/sensor",
            "/api/v2/projects/{projectId}/shards",
            "/api/v2/projects/{projectId}/menu",
            "/api/internal/pins",
            "/api/v3alpha/graph",
            "/api/internal/dashboards",
            "/api/internal/graphs",
            "/api/v2/projects/{projectId}/alerts/{parentId}/subAlerts",
            "/api/v2/agents",
            "/monitoring/v1/metrics/labelKeys/values",
            "/monitoring/v2/data/write",
            "/balancer-ping",
            "/api/internal/pins/{id}"
        );

        resolver = new RequestPatternResolver(endpoints);
    }

    @Test
    public void empty() {
        var result = resolver.resolve("/unknown");
        assertEquals(Optional.empty(), result);
    }

    @Test
    public void resolveExact() {
        var result = resolver.resolve("/push/json");
        assertEquals(Optional.of("/push/json"), result);
    }

    @Test
    public void noConsiderLastSlash() {
        var result = resolver.resolve("/push/json/");
        assertEquals(Optional.of("/push/json"), result);
    }

    @Test
    public void resolvePathVariable() {
        expect("/api/v2/projects/:id", "/api/v2/projects/junk/");
        expect("/api/v2/projects/:projectId/shards/:shardId", "/api/v2/projects/solomon/shards/vlgo");
        expect("/api/internal/projects/:projectId/graphs", "/api/internal/projects/junk/graphs");
        expect(null, "/api/v2/test");
    }

    @Test
    public void compatibility() {
        expect("/monitoring/v1/metrics/labelValues", "/monitoring/v1/metrics/labelValues");
        expect("/monitoring/v1/channels", "/monitoring/v1/channels");
        expect("/push", "/push");
        expect("/api/internal/projects/:projectId/shardLabels", "/api/internal/projects/123/shardLabels");
        expect("/swagger-resources/configuration/ui", "/swagger-resources/configuration/ui");
        expect("/monitoring/v1/data/write", "/monitoring/v1/data/write");
        expect("/api/v2/projects/:projectId/sensors/names", "/api/v2/projects/junk/sensors/names");
        expect("/api/v2/projects/:projectId/alerts/:alertId/state/evaluationStats", "/api/v2/projects/junk/alerts/123-321/state/evaluationStats");
        expect("/monitoring/v1/alerts", "/monitoring/v1/alerts");
        expect("/api/v2/projects/:projectId/dashboards", "/api/v2/projects/junk/dashboards");
        expect("/staffOnly", "/staffOnly");
        expect("/api/internal/settings", "/api/internal/settings");
        expect("/api/v2/projects/:projectId/clusters", "/api/v2/projects/junk/clusters");
        expect("/api/internal/projects/:projectId/dashboards/:dashboardId", "/api/internal/projects/junk/dashboards/{dashboardId}");
        expect("/api/v2/projects/:projectId/services/:serviceId/clusters", "/api/v2/projects/junk/services/{serviceId}/clusters");
        expect("/api/internal/projects/:projectId/matchingShard", "/api/internal/projects/junk/matchingShard");
        expect("/api/internal/projects/:projectId/menu", "/api/internal/projects/junk/menu");
        expect("/swagger-resources/configuration/security", "/swagger-resources/configuration/security");
        expect("/api/v2/projects/:projectId/alerts/:alertId/state/evaluation", "/api/v2/projects/junk/alerts/123-321/state/evaluation");
        expect("/monitoring/v2/metrics/labels/:key/values", "/monitoring/v2/metrics/labels/{123}/values");
        expect("/api/v2/projects/:projectId/dashboards/:dashboardId", "/api/v2/projects/junk/dashboards/{dashboardId}");
        expect("/api/v2/projects/:projectId/alerts/:parentId/subAlerts/:alertId", "/api/v2/projects/junk/alerts/{parentId}/subAlerts/123-321");
        expect("/monitoring/v1/channels/status/history", "/monitoring/v1/channels/status/history");
        expect("/monitoring/v1/metrics/labelKeys", "/monitoring/v1/metrics/labelKeys");
        expect("/api/v2/projects/:projectId/alerts/explainEvaluation", "/api/v2/projects/junk/alerts/explainEvaluation");
        expect("/monitoring/v1/alerts/state/evaluation", "/monitoring/v1/alerts/state/evaluation");
        expect("/api/v2/projects/:projectId/services", "/api/v2/projects/junk/services");
        expect("/api/internal/pinByUrl", "/api/internal/pinByUrl");
        expect("/api/v2/projects", "/api/v2/projects");
        expect("/api/v2/projects/:projectId/alerts/:alertId/state/notificationStats", "/api/v2/projects/junk/alerts/123-321/state/notificationStats");
        expect("/rest/grafana/search", "/rest/grafana/search");
        expect("/monitoring/v1/alerts/evaluation/history", "/monitoring/v1/alerts/evaluation/history");
        expect("/api/v2/telegram/groupTitles", "/api/v2/telegram/groupTitles");
        expect("/api/internal/settings/:key", "/api/internal/settings/my-key");
        expect("/api/v2/projects/:projectId/services/:serviceId", "/api/v2/projects/junk/services/myServiceId");
        expect("/api/v2/push", "/api/v2/push");
        expect("/api/internal/projects/:projectId/graphs", "/api/internal/projects/junk/graphs");
        expect("/monitoring/v1/data/read", "/monitoring/v1/data/read");
        expect("/monitoring/v1/metrics", "/monitoring/v1/metrics");
        expect("/api/v2/projects/:projectId/shards/:shardId", "/api/v2/projects/junk/shards/myShardId");
        expect("/api/v2/projects/:projectId/sensors/labels", "/api/v2/projects/junk/sensors/labels");
        expect("/rest/graph-data", "/rest/graph-data");
        expect("/monitoring/v1/expression/functions", "/monitoring/v1/expression/functions");
        expect("/rest/grafana/sensors/value/list", "/rest/grafana/sensors/value/list");
        expect("/api/v2/projects/:projectId/graphs/:graphId", "/api/v2/projects/junk/graphs/{graphId}");
        expect("/api/v2/projects/:projectId/notificationChannels/:notificationChannelId", "/api/v2/projects/junk/notificationChannels/{notificationChannelId}");
        expect("/api/internal/shards", "/api/internal/shards");
        expect("/push/json", "/push/json");
        expect("/rest/grafana/sensors/key/list", "/rest/grafana/sensors/key/list");
        expect("/api/v2/projects/:projectId/notificationChannels", "/api/v2/projects/junk/notificationChannels");
        expect("/api/v3alpha/graphs", "/api/v3alpha/graphs");
        expect("/api/v2/projects/:projectId/alerts/:alertId", "/api/v2/projects/junk/alerts/123-321");
        expect("/data-api/get", "/data-api/get");
        expect("/api/v2/projects/:projectId/clusters/:clusterId/services", "/api/v2/projects/junk/clusters/myClusterId/services");
        expect("/api/v2/projects/:projectId/alerts/subAlerts/explainEvaluation", "/api/v2/projects/junk/alerts/subAlerts/explainEvaluation");
        expect("/data-api/sensors", "/data-api/sensors");
        expect("/api/v2/projects/:projectId/graphs", "/api/v2/projects/junk/graphs");
        expect("/monitoring/v1/metrics/names", "/monitoring/v1/metrics/names");
        expect("/api/v2/projects/:projectId/alerts/:parentId/subAlerts/:alertId/explainEvaluation", "/api/v2/projects/junk/alerts/myParentId/subAlerts/123-321/explainEvaluation");
        expect("/api/v2/projects/:projectId/alerts/:parentId/subAlerts/:alertId/state/notification", "/api/v2/projects/junk/alerts/myParentId/subAlerts/123-321/state/notification");
        expect("/monitoring/v1/alerts/alert", "/monitoring/v1/alerts/alert");
        expect("/api/v2/projects/:projectId/shards/:shardId/targets", "/api/v2/projects/junk/shards/myShardId/targets");
        expect("/api/internal/projects/:projectId/graphs/:graphId", "/api/internal/projects/junk/graphs/{graphId}");
        expect("/api/v2/projects/:projectId/alerts/:alertId/explainEvaluation", "/api/v2/projects/junk/alerts/123-321/explainEvaluation");
        expect("/api/v2/projects/:projectId/alerts/:parentId/subAlerts/:alertId/state/evaluation", "/api/v2/projects/junk/alerts/myParentId/subAlerts/123-321/state/evaluation");
        expect("/monitoring/v1/alerts/simulation/alert", "/monitoring/v1/alerts/simulation/alert");
        expect("/api/v2/projects/:projectId/clusters/:clusterId", "/api/v2/projects/junk/clusters/{clusterId}");
        expect("/monitoring/v2/metrics/names", "/monitoring/v2/metrics/names");
        expect("/api/v2/projects/:projectId/sensors/data", "/api/v2/projects/junk/sensors/data");
        expect("/api/v2/projects/:projectId/alerts", "/api/v2/projects/junk/alerts");
        expect("/api/v3alpha/dashboards", "/api/v3alpha/dashboards");
        expect("/monitoring/v2/metrics", "/monitoring/v2/metrics");
        expect("/api/internal/fetcher/resolveCluster", "/api/internal/fetcher/resolveCluster");
        expect("/api/internal/projects/:projectId/shards/:shardId/reload", "/api/internal/projects/junk/shards/myShardId/reload");
        expect("/api/v2/projects/:projectId/sensors/sensorNames", "/api/v2/projects/junk/sensors/sensorNames");
        expect("/api/v2/projects/:projectId/sensors", "/api/v2/projects/junk/sensors");
        expect("/api/v2/projects/:projectId/alerts/:alertId/state/notification", "/api/v2/projects/junk/alerts/123-321/state/notification");
        expect("/staffOnly/:address/__", "/staffOnly/localhost/**");
        expect("/api/v3alpha/dashboard", "/api/v3alpha/dashboard");
        expect("/api/v2/projects/:id", "/api/v2/projects/junk");
        expect("/api/internal/services", "/api/internal/services");
        expect("/api/internal/projects/:projectId/dashboards", "/api/internal/projects/:projectId/dashboards");
        expect("/rest/grafana/sensors/search", "/rest/grafana/sensors/search");
        expect("/rest/generic", "/rest/generic");
        expect("/api/v2/projects/:projectId/alerts/stats", "/api/v2/projects/junk/alerts/stats");
        expect("/rest/selectorValues", "/rest/selectorValues");
        expect("/monitoring/v1/alerts/state/notification", "/monitoring/v1/alerts/state/notification");
        expect("/swagger-resources", "/swagger-resources");
        expect("/api/internal/clusters", "/api/internal/clusters");
        expect("/api/internal/oauth", "/api/internal/oauth");
        expect("/monitoring/v1/channels/channel", "/monitoring/v1/channels/channel");
        expect("/api/internal/auth", "/api/internal/auth");
        expect("/monitoring/v2/data/read", "/monitoring/v2/data/read");
        expect("/monitoring/v2/metrics/labels", "/monitoring/v2/metrics/labels");
        expect("/api/internal/roles", "/api/internal/roles");
        expect("/api/internal/projects/:projectId/stockpile/sensor", "/api/internal/projects/junk/stockpile/sensor");
        expect("/api/v2/projects/:projectId/shards", "/api/v2/projects/junk/shards");
        expect("/api/v2/projects/:projectId/menu", "/api/v2/projects/junk/menu");
        expect("/api/internal/pins", "/api/internal/pins");
        expect("/api/v3alpha/graph", "/api/v3alpha/graph");
        expect("/api/internal/dashboards", "/api/internal/dashboards");
        expect("/api/internal/graphs", "/api/internal/graphs");
        expect("/api/v2/projects/:projectId/alerts/:parentId/subAlerts", "/api/v2/projects/junk/alerts/myParentId/subAlerts");
        expect("/api/v2/agents", "/api/v2/agents");
        expect("/monitoring/v1/metrics/labelKeys/values", "/monitoring/v1/metrics/labelKeys/values");
        expect("/monitoring/v2/data/write", "/monitoring/v2/data/write");
        expect("/balancer-ping", "/balancer-ping");
        expect("/api/internal/pins/:id", "/api/internal/pins/123");
    }

    private void expect(String expect, String uri) {
        {
            var result = resolver.resolve(uri);
            assertEquals(Optional.ofNullable(expect), result);
        }
        {
            var result = resolver.resolve(uri + "/");
            assertEquals(Optional.ofNullable(expect), result);
        }
    }
}

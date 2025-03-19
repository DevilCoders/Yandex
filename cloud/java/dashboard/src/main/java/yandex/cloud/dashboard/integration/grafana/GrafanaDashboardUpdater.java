package yandex.cloud.dashboard.integration.grafana;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.github.fge.jsonpatch.diff.JsonDiff;
import com.google.common.base.Preconditions;
import lombok.Data;
import lombok.SneakyThrows;
import lombok.extern.log4j.Log4j2;
import yandex.cloud.dashboard.integration.common.HttpResponseException;
import yandex.cloud.dashboard.model.result.dashboard.Dashboard;
import yandex.cloud.dashboard.model.result.dashboard.DashboardData;
import yandex.cloud.dashboard.util.Json;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.StreamSupport;

import static com.google.common.base.MoreObjects.firstNonNull;

/**
 * @author akirakozov
 */
@Log4j2
public class GrafanaDashboardUpdater {
    private final GrafanaClient grafanaClient = new GrafanaClient();

    @SuppressWarnings({"ConstantConditions"})
    public void createUpdate(DashboardData dashboardData, boolean dryRun) {
        try {
            update(dashboardData, dryRun);
        } catch (Exception e) {
            if (e instanceof HttpResponseException && ((HttpResponseException) e).getCode() == 404) {
                log.info("Existing dashboard not found by uid '{}'", dashboardData.getDashboard().getUid());
                if (dryRun) {
                    log.info("Dashboard won't be created at Grafana due to DRY RUN");
                } else {
                    grafanaClient.createUpdateDashboard(Json.toJson(dashboardData));
                    log.info("Dashboard was created at Grafana, please check out:");
                    log.info(grafanaClient.getUiDashboardUrl(dashboardData.getDashboard().getUid()));
                }
            } else {
                throw e;
            }
        }
    }

    public void update(DashboardData newData, boolean dryRun) {
        Dashboard newDashboard = newData.getDashboard();
        String uid = newDashboard.getUid();
        Preconditions.checkNotNull(uid, "Dashboard uid should be specified");

        String remoteRaw = grafanaClient.getDashboard(uid);
        Preconditions.checkState(!remoteRaw.startsWith("<!doctype html>"),
                "Grafana returned html response instead of json, please verify the Grafana OAuth token");
        RemoteDashboardData remoteDashboardData = Json.fromJson(RemoteDashboardData.class, remoteRaw);

        long remoteId = remoteDashboardData.getDashboard().getId();
        long remoteVersion = remoteDashboardData.getDashboard().getVersion();
        if (newDashboard.getId() == null) {
            newDashboard.setId(remoteId);
        } else {
            Preconditions.checkState(newDashboard.getId() == remoteId,
                    "Id mismatch: expected %s, but for uid '%s', remote id is %s", newDashboard.getId(), uid, remoteId);
        }
        newDashboard.setVersion(remoteVersion);

        String newRaw = Json.toJson(newData);
        List<JsonNode> diff = calcDiff(remoteRaw, newRaw);

        long remoteFolderId = remoteDashboardData.getMeta().getFolderId();
        long newFolderId = firstNonNull(newData.getFolderId(), 0L);

        if (diff.isEmpty() && (newFolderId == remoteFolderId)) {
            log.info("Nothing to change (version {})", remoteVersion);
        } else {
            log.info("Need to update dashboard '{}' (version {}) at Grafana, changes are:", uid, remoteVersion);
            if (newFolderId != remoteFolderId) {
                log.info("{\"folderId\":{}}", newFolderId);
            }
            diff.forEach(log::info);
            if (dryRun) {
                log.info("Dashboard won't update at Grafana due to DRY RUN");
            } else {
                grafanaClient.createUpdateDashboard(Json.toJson(newData));
                log.info("Dashboard was updated at Grafana (to version {}), please check out:", remoteVersion + 1);
                log.info(grafanaClient.getUiDashboardUrl(uid));
            }
        }
    }

    @SneakyThrows
    private static List<JsonNode> calcDiff(String remoteRaw, String newRaw) {
        ObjectMapper jackson = new ObjectMapper();
        JsonNode remoteNode = jackson.readTree(remoteRaw);
        JsonNode newNode = jackson.readTree(newRaw);
        JsonNode diff = JsonDiff.asJson(remoteNode, newNode);
        return StreamSupport.stream(diff.spliterator(), false)
                .filter(n -> n.get("path").textValue().startsWith("/dashboard"))
                .collect(Collectors.toList());
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    @Data
    private static class RemoteMeta {
        long folderId;
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    @Data
    private static class RemoteDashboard {
        long id;
        long version;
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    @Data
    private static class RemoteDashboardData {
        RemoteMeta meta;
        RemoteDashboard dashboard;
    }
}

package yandex.cloud.dashboard.model.result.dashboard;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.With;

/**
 * To discover (optional) folder id, one can use:
 * <pre>
 * curl -H "Content-type: application-json" -H "Authorization: OAuth $GRAFANA_OAUTH_TOKEN" \
 * "https://grafana.yandex-team.ru/api/folders?limit=10" | jq .
 * </pre>
 *
 * @author akirakozov
 */
@AllArgsConstructor
@With
@Data
public class DashboardData {
    Long folderId;
    Dashboard dashboard;
    String message;
}

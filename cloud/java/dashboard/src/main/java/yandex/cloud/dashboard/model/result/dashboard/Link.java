package yandex.cloud.dashboard.model.result.dashboard;

import lombok.AllArgsConstructor;
import lombok.Data;
import yandex.cloud.dashboard.integration.grafana.GrafanaClient;
import yandex.cloud.dashboard.model.spec.dashboard.LinkSpec;

import java.util.List;

import static com.google.common.base.MoreObjects.firstNonNull;

/**
 * @author ssytnik
 */
@AllArgsConstructor
@Data
public class Link {
    Boolean asDropdown;
    String dashboard;
    String icon;
    Boolean includeVars;
    Boolean keepTime;
    List<String> tags;
    boolean targetBlank;
    String title;
    String type;
    String url;

    public static Link createFromDashboardLink(LinkSpec ls) {
        if (ls.isDashboardToDashboardList()) {
            return createLink(ls.getTitle() != null, null, "dashboard", ls.getVars(), ls.getTags(), ls.getTargetBlank(), ls.getTitle(), "dashboards", null);
        } else if (ls.isExternal()) {
            return createLink(null, null, "external link", ls.getVars(), List.of(), ls.getTargetBlank(), ls.getTitle(), "link", ls.getUrl());
        } else {
            throw new IllegalArgumentException("Wrong from-dashboard " + ls);
        }
    }

    public static Link createFromGraphLink(LinkSpec ls) {
        if (ls.isGraphToDashboard()) {
            return createLink(null, ls.getTitle(), null, ls.getVars(), null, ls.getTargetBlank(), ls.getTitle(), "dashboard", GrafanaClient.getUiDashboardRelativeUrl(ls.getUid()));
        } else if (ls.isExternal()) {
            return createLink(null, null, null, ls.getVars(), null, ls.getTargetBlank(), ls.getTitle(), "absolute", ls.getUrl());
        } else {
            throw new IllegalArgumentException("Wrong from-graph " + ls);
        }
    }

    private static Link createLink(Boolean asDropdown, String dashboard, String icon, Boolean vars0, List<String> tags, Boolean targetBlank, String title, String type, String url) {
        boolean vars = firstNonNull(vars0, true);
        return new Link(asDropdown, dashboard, icon, vars, vars, tags, firstNonNull(targetBlank, false), title, type, url);
    }
}

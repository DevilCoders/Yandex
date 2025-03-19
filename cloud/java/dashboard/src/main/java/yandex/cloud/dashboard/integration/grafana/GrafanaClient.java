package yandex.cloud.dashboard.integration.grafana;

import lombok.SneakyThrows;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpRequestBase;
import org.apache.http.entity.StringEntity;
import yandex.cloud.dashboard.integration.common.AbstractHttpClient;
import yandex.cloud.dashboard.integration.common.DefaultResponseHandler;

import static java.nio.charset.StandardCharsets.UTF_8;

/**
 * To take token for our app use link
 * https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cfa5d75ea95f4ae594acbdaf8ca6770c
 *
 * @author akirakozov
 */
public class GrafanaClient extends AbstractHttpClient {
    private static final String DEFAULT_HOST = "grafana.yandex-team.ru";
    private final String oauthToken;

    public GrafanaClient() {
        this(DEFAULT_HOST, System.getenv("GRAFANA_OAUTH_TOKEN"));
    }

    public GrafanaClient(String host, String oauthToken) {
        super(host);
        this.oauthToken = oauthToken;
    }

    @SneakyThrows
    public String getDashboard(String uid) {
        HttpGet httpGet = new HttpGet("https://" + host + "/api/dashboards/uid/" + uid);
        addAuthorization(httpGet);
        return httpClient.execute(httpGet, new DefaultResponseHandler());
    }

    // TODO move to proper place
    public String getUiDashboardUrl(String uid) {
        return "https://" + host + getUiDashboardRelativeUrl(uid);
    }

    // TODO move to proper place
    public static String getUiDashboardRelativeUrl(String uid) {
        return "/d/" + uid;
    }

    @SneakyThrows
    public String createUpdateDashboard(String dashboardData) {
        HttpPost httpPost = new HttpPost("https://" + host + "/api/dashboards/db");
        addAuthorization(httpPost);
        httpPost.addHeader("Content-Type", "application/json");
        httpPost.setEntity(new StringEntity(dashboardData, UTF_8));
        return httpClient.execute(httpPost, new DefaultResponseHandler());
    }

    @SneakyThrows
    public String deleteDashboard(String uid) {
        HttpDelete httpDelete = new HttpDelete("https://" + host + "/api/dashboards/uid/" + uid);
        addAuthorization(httpDelete);
        httpDelete.addHeader("Content-Type", "application/json");
        return httpClient.execute(httpDelete, new DefaultResponseHandler());
    }

    private void addAuthorization(HttpRequestBase request) {
        request.addHeader("Authorization", "OAuth " + oauthToken);
    }
}

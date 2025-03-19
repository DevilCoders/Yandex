package yandex.cloud.dashboard.integration.grafana;

import org.junit.Ignore;
import org.junit.Test;
import yandex.cloud.util.RandomUtils;

import static org.assertj.core.api.Assertions.assertThat;

/**
 * @author akirakozov
 */
@Ignore("For manual run only")
public class GrafanaClientTest {

    @Test
    public void getDashboard() {
        GrafanaClient client = new GrafanaClient();
        System.out.println(client.getDashboard("cZdTlelmz"));
    }

    @Test
    public void updateDashboard() {
        GrafanaClient client = new GrafanaClient();
        System.out.println(client.createUpdateDashboard(testDashboardDataForUpdate()));
    }

    @Test
    public void createDashboard() {
        String uid = RandomUtils.nextString(9);
        String title = "test title " + RandomUtils.nextString(6);
        GrafanaClient client = new GrafanaClient();
        try {
            String response = client.createUpdateDashboard(testDataDashboadForCreate(uid, title));
            System.out.println(response);
            assertThat(response).contains("\"uid\":\"" + uid + "\"");
            assertThat(response).contains("\"version\":1");
        } finally {
            String response = client.deleteDashboard(uid);
            System.out.println(response);
        }
    }

    @Test
    public void deleteDashboard() {
        GrafanaClient client = new GrafanaClient();
        System.out.println(client.deleteDashboard("0p9l1POsh"));
    }


    private String testDashboardDataForUpdate() {
        return "{\"meta\":{\"type\":\"db\",\"canSave\":true,\"canEdit\":true,\"canAdmin\":false,\"canStar\":true,\"slug\":\"test-grafana\",\"url\":\"/d/cZdTlelmz/test-grafana\",\"expires\":\"0001-01-01T00:00:00Z\",\"created\":\"2019-02-04T23:11:56+03:00\",\"updated\":\"2019-02-04T23:11:56+03:00\",\"updatedBy\":\"akirakozov\",\"createdBy\":\"akirakozov\",\"version\":2,\"hasAcl\":false,\"isFolder\":false,\"folderId\":0,\"folderTitle\":\"General\",\"folderUrl\":\"\",\"provisioned\":false},\"dashboard\":{\"annotations\":{\"list\":[{\"builtIn\":1,\"datasource\":\"-- Grafana --\",\"enable\":true,\"hide\":true,\"iconColor\":\"rgba(0, 211, 255, 1)\",\"name\":\"Annotations \\u0026 Alerts\",\"type\":\"dashboard\"}]},\"editable\":true,\"gnetId\":null,\"graphTooltip\":0,\"hideControls\":false,\"id\":86987,\"links\":[],\"panels\":[{\"aliasColors\":{\"A-series\":\"#aa12e2\"},\"bars\":false,\"dashLength\":10,\"dashes\":false,\"fill\":1,\"gridPos\":{\"h\":9,\"w\":12,\"x\":0,\"y\":0},\"id\":2,\"legend\":{\"avg\":false,\"current\":false,\"max\":false,\"min\":false,\"show\":true,\"total\":false,\"values\":false},\"lines\":true,\"linewidth\":1,\"nullPointMode\":\"null\",\"percentage\":false,\"pointradius\":5,\"points\":false,\"renderer\":\"flot\",\"seriesOverrides\":[],\"spaceLength\":10,\"stack\":false,\"steppedLine\":false,\"targets\":[{\"refId\":\"A\"}],\"thresholds\":[],\"timeFrom\":null,\"timeRegions\":[],\"timeShift\":null,\"title\":\"Panel Title\",\"tooltip\":{\"shared\":true,\"sort\":0,\"value_type\":\"individual\"},\"type\":\"graph\",\"xaxis\":{\"buckets\":null,\"mode\":\"time\",\"name\":null,\"show\":true,\"values\":[]},\"yaxes\":[{\"format\":\"short\",\"label\":null,\"logBase\":1,\"max\":null,\"min\":null,\"show\":true},{\"format\":\"short\",\"label\":null,\"logBase\":1,\"max\":null,\"min\":null,\"show\":true}],\"yaxis\":{\"align\":false,\"alignLevel\":null}}],\"schemaVersion\":16,\"style\":\"dark\",\"tags\":[],\"templating\":{\"list\":[]},\"time\":{\"from\":\"now-6h\",\"to\":\"now\"},\"timepicker\":{\"refresh_intervals\":[\"5s\",\"10s\",\"30s\",\"1m\",\"5m\",\"15m\",\"30m\",\"1h\",\"2h\",\"1d\"],\"time_options\":[\"5m\",\"15m\",\"1h\",\"6h\",\"12h\",\"24h\",\"2d\",\"7d\",\"30d\"]},\"timezone\":\"\",\"title\":\"test-grafana\",\"uid\":\"cZdTlelmz\",\"version\":3}}";
    }

    private String testDataDashboadForCreate(String uid, String title) {
        return "{\"meta\":{\"type\":\"db\",\"canSave\":true,\"canEdit\":true,\"canAdmin\":false,\"canStar\":true,\"slug\":\"test-grafana\",\"url\":\"/d/cZdTlelmz/test-grafana\",\"expires\":\"0001-01-01T00:00:00Z\",\"created\":\"2019-02-04T23:11:56+03:00\",\"updated\":\"2019-02-04T23:11:56+03:00\",\"updatedBy\":\"akirakozov\",\"createdBy\":\"akirakozov\",\"version\":2,\"hasAcl\":false,\"isFolder\":false,\"folderId\":0,\"folderTitle\":\"General\",\"folderUrl\":\"\",\"provisioned\":false},\"dashboard\":{\"annotations\":{\"list\":[{\"builtIn\":1,\"datasource\":\"-- Grafana --\",\"enable\":true,\"hide\":true,\"iconColor\":\"rgba(0, 211, 255, 1)\",\"name\":\"Annotations \\u0026 Alerts\",\"type\":\"dashboard\"}]},\"editable\":true,\"gnetId\":null,\"graphTooltip\":0,\"hideControls\":false,\"links\":[],\"panels\":[{\"aliasColors\":{\"A-series\":\"#aa12e2\"},\"bars\":false,\"dashLength\":10,\"dashes\":false,\"fill\":1,\"gridPos\":{\"h\":9,\"w\":12,\"x\":0,\"y\":0},\"id\":2,\"legend\":{\"avg\":false,\"current\":false,\"max\":false,\"min\":false,\"show\":true,\"total\":false,\"values\":false},\"lines\":true,\"linewidth\":1,\"nullPointMode\":\"null\",\"percentage\":false,\"pointradius\":5,\"points\":false,\"renderer\":\"flot\",\"seriesOverrides\":[],\"spaceLength\":10,\"stack\":false,\"steppedLine\":false,\"targets\":[{\"refId\":\"A\"}],\"thresholds\":[],\"timeFrom\":null,\"timeRegions\":[],\"timeShift\":null,\"title\":\"Panel Title\",\"tooltip\":{\"shared\":true,\"sort\":0,\"value_type\":\"individual\"},\"type\":\"graph\",\"xaxis\":{\"buckets\":null,\"mode\":\"time\",\"name\":null,\"show\":true,\"values\":[]},\"yaxes\":[{\"format\":\"short\",\"label\":null,\"logBase\":1,\"max\":null,\"min\":null,\"show\":true},{\"format\":\"short\",\"label\":null,\"logBase\":1,\"max\":null,\"min\":null,\"show\":true}],\"yaxis\":{\"align\":false,\"alignLevel\":null}}],\"schemaVersion\":16,\"style\":\"dark\",\"tags\":[],\"templating\":{\"list\":[]},\"time\":{\"from\":\"now-6h\",\"to\":\"now\"},\"timepicker\":{\"refresh_intervals\":[\"5s\",\"10s\",\"30s\",\"1m\",\"5m\",\"15m\",\"30m\",\"1h\",\"2h\",\"1d\"],\"time_options\":[\"5m\",\"15m\",\"1h\",\"6h\",\"12h\",\"24h\",\"2d\",\"7d\",\"30d\"]},\"timezone\":\"\",\"title\":\"" + title + "\",\"uid\":\"" + uid + "\"}}";
    }

}
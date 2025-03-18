package admin_test

import (
	"context"
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/ptr"
	"a.yandex-team.ru/library/go/yandex/oauth"
	"a.yandex-team.ru/library/go/yandex/solomon"
	"a.yandex-team.ru/library/go/yandex/solomon/admin"
)

func testClient(t *testing.T) *admin.Client {
	project := os.Getenv("SOLOMON_TEST_PROJECT")
	if project == "" {
		t.Log("Create test project at https://solomon-prestable.yandex-team.ru and set SOLOMON_TEST_PROJECT environment variable")
		t.Skipf("This test must be executed manually")
	}

	oauthToken, err := oauth.GetTokenBySSH(context.Background(), admin.SolomonCLIID, admin.SolomonCLISecret)
	require.NoError(t, err)

	return admin.New(project, oauthToken, admin.WithURL(admin.PrestableURL), admin.WithDebugLog())
}

func cleanup(t *testing.T, client solomon.AdminClient) {
	shards, err := client.ListShards(context.Background())
	require.NoError(t, err)
	for _, s := range shards {
		require.NoError(t, client.DeleteShard(context.Background(), s.ID))
	}

	clusters, err := client.ListClusters(context.Background(), solomon.PageSize{Size: nil})
	require.NoError(t, err)
	for _, c := range clusters {
		require.NoError(t, client.DeleteCluster(context.Background(), c.ID))
	}

	services, err := client.ListServices(context.Background(), solomon.PageSize{Size: nil})
	require.NoError(t, err)
	for _, s := range services {
		require.NoError(t, client.DeleteService(context.Background(), s.ID))
	}

	alerts, err := client.ListAlerts(context.Background(), solomon.PageSize{Size: ptr.Int(1000)})
	require.NoError(t, err)
	for _, a := range alerts {
		require.NoError(t, client.DeleteAlert(context.Background(), a.ID))
	}
}

func TestAdmin(t *testing.T) {
	client := testClient(t)

	cleanup(t, client)

	_, err := client.CreateCluster(context.Background(), solomon.Cluster{
		ID:   "test_cluster",
		Name: "test_cluster",
	})
	require.NoError(t, err)

	_, err = client.CreateService(context.Background(), solomon.Service{
		ID:   "test_service",
		Name: "test_service",
		Port: 8080,
		Path: "/debug/solomon",
	})
	require.NoError(t, err)

	_, err = client.CreateShard(context.Background(), solomon.Shard{
		ID:        "test_shard",
		ClusterID: "test_cluster",
		ServiceID: "test_service",
	})
	require.NoError(t, err)

	cluster, err := client.FindCluster(context.Background(), "test_cluster")
	require.NoError(t, err)
	require.Equal(t, "test_cluster", cluster.Name)

	cluster.HostURLs = []solomon.HostURL{{URL: "localhost:12345/xsrf"}}

	_, err = client.UpdateCluster(context.Background(), cluster)
	require.NoError(t, err)

	service, err := client.FindService(context.Background(), "test_service")
	require.NoError(t, err)
	require.Equal(t, "test_service", service.Name)

	service.Path = "/pbjson"
	_, err = client.UpdateService(context.Background(), service)
	require.NoError(t, err)

	cleanup(t, client)
}

func TestAlerts(t *testing.T) {
	client := testClient(t)

	cleanup(t, client)

	const N = 20
	for i := 0; i < N; i++ {
		id := fmt.Sprintf("alert_%d", i)
		_, err := client.CreateAlert(context.Background(), solomon.Alert{
			ID:           id,
			Name:         id,
			PeriodMillis: 1000,
			Type: solomon.AlertType{
				Threshold: &solomon.AlertThreshold{
					Selectors: "sensor=test",
					PredicateRules: []solomon.AlertPredicateRule{
						{Threshold: 0, Comparison: "GTE", ThresholdType: "MAX", TargetStatus: "WARN"},
					},
				},
			},
		})

		require.NoError(t, err)
	}

	all, err := solomon.ListAllAlerts(context.Background(), client, solomon.AlertFilter{FilterByName: "alert_"})
	require.NoError(t, err)

	require.Equal(t, N, len(all))
}

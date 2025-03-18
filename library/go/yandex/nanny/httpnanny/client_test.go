package httpnanny

import (
	"context"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/x/encoding/unknownjson"
	"a.yandex-team.ru/library/go/yandex/nanny"
)

const testCategory = "/nanny_gotest/"

type testClient struct {
	nanny.Client
}

func newClient(t *testing.T) *testClient {
	t.Helper()

	token := os.Getenv("NANNY_TOKEN")
	if token == "" {
		t.Skip("NANNY_TOKEN is not set")
	}

	c, err := New(WithToken(token), WithDev(), WithDebug())
	require.NoError(t, err)

	return &testClient{Client: c}
}

func (c *testClient) cleanup(t *testing.T) {
	t.Helper()

	ctx := context.Background()

	services, err := c.ListServices(ctx, nanny.ListOptions{Category: testCategory, ExcludeRuntimeAttrs: true})
	require.NoError(t, err)

	for _, s := range services {
		require.NoError(t, c.DeleteService(ctx, s.ID))
	}
}

var (
	ubuntuLayer = nanny.LayersConfig{
		Layer: []nanny.Resource{
			{
				URL: []string{"rbtorrent:e216e9855668c9c9eb059f29f97c0f7d67fecdea"},
				Meta: nanny.Meta{
					Type: "SANDBOX_RESOURCE",
					SandboxResource: &nanny.SandboxResource{
						ResourceType: "PORTO_LAYER",
						ResourceID:   627125073,
						TaskType:     "BUILD_PORTO_LAYER",
						TaskID:       278153199,
					},
				},
			},
		},
	}

	testService = &nanny.ServiceSpec{
		ID: "nanny_go_test",

		InfoAttrs: &nanny.InfoAttrs{
			Description: "test service created by a.yandex-team.ru/library/go/yandex/nanny",
			Category:    testCategory,
			ABCService:  470, // YT
		},

		RuntimeAttrs: &nanny.RuntimeAttrs{
			Engine: nanny.EngineYPLite,

			Instances: nanny.Instances{
				Type: nanny.InstanceTypeYPPods,
			},

			InstanceSpec: nanny.InstanceSpec{
				LayersConfig: ubuntuLayer,
			},
		},

		AuthAttrs: &nanny.AuthAttrs{},
	}
)

func TestJSON(t *testing.T) {
	_, err := unknownjson.Marshal(testService)
	require.NoError(t, err)
}

func TestClient(t *testing.T) {
	c := newClient(t)
	c.cleanup(t)

	ctx := context.Background()

	checkService := func(s *nanny.Service) {
		t.Helper()

		require.Equal(t, testService.ID, s.ID)

		s.Info.Attrs.Unknown = unknownjson.Store{}
		assert.Equal(t, testService.InfoAttrs, s.Info.Attrs)

		// Remove whatever login was set by default.
		s.Auth.Attrs.Owners.Logins = nil
		s.Auth.Attrs.ConfManagers.Logins = nil
		s.Auth.Attrs.OpsManagers.Logins = nil
		assert.Equal(t, testService.AuthAttrs, s.Auth.Attrs)

		s.Runtime.Attrs.InstanceSpec.Type = ""
		s.Runtime.Attrs.InstanceSpec.InstanceCtl = nil
		s.Runtime.Attrs.Resources.Unknown = unknownjson.Store{}
		assert.Equal(t, testService.RuntimeAttrs, s.Runtime.Attrs)
	}

	service, err := c.CreateService(ctx, testService, "create new service")
	require.NoError(t, err)
	checkService(service)

	allServices, err := c.ListServices(ctx, nanny.ListOptions{Category: testCategory})
	require.NoError(t, err)
	require.Len(t, allServices, 1)

	checkService(&allServices[0])

	newService, err := c.GetService(ctx, service.ID)
	require.NoError(t, err)

	_, err = c.UpdateAuthAttrs(ctx, testService.ID, newService.Auth.Attrs, newService.Auth.UpdateWithComment("testing auth update"))
	require.NoError(t, err)

	_, err = c.UpdateRuntimeAttrs(ctx, testService.ID, newService.Runtime.Attrs, newService.Runtime.UpdateWithComment("testing runtime update"))
	require.NoError(t, err)

	_, err = c.UpdateInfoAttrs(ctx, testService.ID, newService.Info.Attrs, newService.Info.UpdateWithComment("testing info update"))
	require.NoError(t, err)

	checkService(newService)

	auth, err := c.GetAuthAttrs(ctx, testService.ID)
	require.NoError(t, err)

	_, err = c.UpdateAuthAttrs(ctx, testService.ID, auth.Attrs, auth.UpdateWithComment("testing auth update"))
	require.NoError(t, err)

	runtime, err := c.GetRuntimeAttrs(ctx, testService.ID)
	require.NoError(t, err)

	_, err = c.UpdateRuntimeAttrs(ctx, testService.ID, runtime.Attrs, runtime.UpdateWithComment("testing runtime update"))
	require.NoError(t, err)

	info, err := c.GetInfoAttrs(ctx, testService.ID)
	require.NoError(t, err)

	_, err = c.UpdateInfoAttrs(ctx, testService.ID, info.Attrs, info.UpdateWithComment("testing info update"))
	require.NoError(t, err)
}

func TestClientErrors(t *testing.T) {
	c := newClient(t)

	ctx := context.Background()

	_, err := c.GetService(ctx, "nanny_go_missing")
	require.Error(t, err)
	require.Truef(t, nanny.IsNotFound(err), "%v", err)
}

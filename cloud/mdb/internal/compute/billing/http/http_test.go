package http

import (
	"context"
	"encoding/json"
	"io/ioutil"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/compute/billing"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/ptr"
)

func initTest(t *testing.T) (context.Context, log.Logger) {
	ctx := context.Background()

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)
	return ctx, l
}

func TestClient_MetricsRequest(t *testing.T) {
	ctx, lg := initTest(t)

	testCases := []struct {
		desc     string
		instance billing.ComputeInstance
		expected string
	}{{
		"struct with nils",
		billing.ComputeInstance{
			FolderID:   "folder1",
			PlatformID: "platform1",
			ZoneID:     nil,
			Resources: billing.ComputeInstanceResources{
				Memory:       4294967296,
				Cores:        1,
				CoreFraction: 100,
				GPUs:         0,
			},
			SubnetID: nil,
			BootDisk: billing.ComputeDisk{
				TypeID:  ptr.String("type1"),
				Size:    ptr.Uint64(10737418240),
				ImageID: "image1",
			},
		},
		`{"folder_id":"folder1","platform_id":"platform1","resources_spec":{"memory":4294967296,"cores":1,"core_fraction":100,"gpus":0},"boot_disk_spec":{"disk_spec":{"type_id":"type1","size":10737418240,"image_id":"image1"}},"network_interface_specs":[{}]}`,
	},
		{
			"full filled struct",
			billing.ComputeInstance{
				FolderID:   "folder1",
				PlatformID: "platform1",
				ZoneID:     ptr.String("zone1"),
				Resources: billing.ComputeInstanceResources{
					Memory:       4294967296,
					Cores:        1,
					CoreFraction: 100,
					GPUs:         0,
				},
				SubnetID: ptr.String("subnet1"),
				BootDisk: billing.ComputeDisk{
					TypeID:  ptr.String("type1"),
					Size:    ptr.Uint64(10737418240),
					ImageID: "image1",
				},
			},
			`{"folder_id":"folder1","platform_id":"platform1","resources_spec":{"memory":4294967296,"cores":1,"core_fraction":100,"gpus":0},"boot_disk_spec":{"disk_spec":{"type_id":"type1","size":10737418240,"image_id":"image1"}},"zone_id":"zone1","network_interface_specs":[{"subnet_id":"subnet1"}]}`,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.desc, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				body, err := ioutil.ReadAll(r.Body)
				assert.NoError(t, err)
				assert.JSONEq(t, tc.expected, string(body))
			}))
			defer srv.Close()

			client, err := New(srv.URL, lg)
			assert.NoError(t, err)
			_, _ = client.Metrics(ctx, tc.instance)
		})
	}
}

func TestClient_MetricsResponse(t *testing.T) {
	ctx, lg := initTest(t)

	testCases := []struct {
		desc     string
		response string
		expected []billing.Metric
	}{{
		"full response",
		`{"metrics": [
			{
                "folder_id": "folder1",
                "schema": "nbs.volume.allocated.v1",
                "tags": {
                    "size": 10737418240,
                    "type": "network-nvme"
                }
            },
            {
                "folder_id": "folder1",
                "schema": "compute.vm.generic.v1",
                "tags": {
                    "cores": 1.0,
                    "memory": 4294967296,
                    "platform_id": "1",
                    "product_ids": ["dummy-product-id"],
                    "public_fips": 0,
                    "sockets": 1
                }
            }]}`,
		[]billing.Metric{
			{
				FolderID: "folder1",
				Schema:   "nbs.volume.allocated.v1",
				Tags: map[string]json.RawMessage{
					"size": []byte(`10737418240`),
					"type": []byte(`"network-nvme"`),
				},
			},
			{
				FolderID: "folder1",
				Schema:   "compute.vm.generic.v1",
				Tags: map[string]json.RawMessage{
					"cores":       []byte(`1.0`),
					"memory":      []byte(`4294967296`),
					"platform_id": []byte(`"1"`),
					"product_ids": []byte(`["dummy-product-id"]`),
					"public_fips": []byte(`0`),
					"sockets":     []byte(`1`),
				},
			}},
	}}

	for _, tc := range testCases {
		t.Run(tc.desc, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, err := w.Write([]byte(tc.response))
				assert.NoError(t, err)
			}))
			defer srv.Close()

			client, err := New(srv.URL, lg)
			assert.NoError(t, err)

			metrics, err := client.Metrics(ctx, billing.ComputeInstance{})
			assert.NoError(t, err)
			assert.Equal(t, tc.expected, metrics)
		})
	}
}

func TestClient_MetricsCache(t *testing.T) {
	ctx, lg := initTest(t)

	testCases := []struct {
		desc     string
		instance billing.ComputeInstance
		resp     string
		metrics  []billing.Metric
	}{
		{
			"cache",
			billing.ComputeInstance{
				FolderID: "folder1",
			},
			`{"metrics": [{"folder_id": "folder1"}]}`,
			[]billing.Metric{{FolderID: "folder1"}},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.desc, func(t *testing.T) {
			isCalled := false
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if isCalled {
					t.Errorf("unexpected call")
				}
				_, err := w.Write([]byte(tc.resp))
				assert.NoError(t, err)
				isCalled = true
			}))
			defer srv.Close()

			client, err := New(srv.URL, lg)
			assert.NoError(t, err)

			for i := 0; i < 3; i++ {
				metrics1, err := client.Metrics(ctx, tc.instance)
				assert.NoError(t, err)
				assert.Equal(t, tc.metrics, metrics1)
			}
		})
	}
}

package http

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestGetMacrosWithOwners(t *testing.T) {
	responseJSON := `{
    "_DBAASEXTERNALNETS_": {
        "owners": [
            {
                "type": "servicerole",
                "name": "svc_ycsecurity_administration"
            }
        ]
    },
    "_MDB_CONTROLPLANE_PORTO_TEST_NETS_": {
        "owners": [
            {
                "type": "user",
                "name": "velom"
            },
            {
                "type": "service",
                "name": "svc_internalmdb"
            }
        ]
    },
    "_MDB_DATAPROC_TEST_NETS_": {
        "owners": [
            {
                "type": "service",
                "name": "svc_dataprocessing"
            },
            {
                "type": "service",
                "name": "svc_ycsecurity"
            }
        ]
    }
}`
	testServer := httptest.NewServer(http.HandlerFunc(func(res http.ResponseWriter, req *http.Request) {
		res.WriteHeader(http.StatusOK)
		if _, err := res.Write([]byte(responseJSON)); err != nil {
			return
		}
	}))
	defer func() { testServer.Close() }()

	logger := &nop.Logger{}

	testClient, err := NewClient(DefaultConfig(), logger)
	require.NoError(t, err)

	testClient.config.Endpoint = testServer.URL

	macrosWithOwners, err := testClient.GetMacrosWithOwners(context.Background())
	require.NoError(t, err)

	expected := racktables.MacrosWithOwners{
		"_DBAASEXTERNALNETS_": []racktables.Owner{
			{Type: "servicerole", Name: "svc_ycsecurity_administration"},
		},
		"_MDB_CONTROLPLANE_PORTO_TEST_NETS_": []racktables.Owner{
			{Type: "user", Name: "velom"},
			{Type: "service", Name: "svc_internalmdb"},
		},
		"_MDB_DATAPROC_TEST_NETS_": []racktables.Owner{
			{Type: "service", Name: "svc_dataprocessing"},
			{Type: "service", Name: "svc_ycsecurity"},
		},
	}

	assert.Equal(t, expected, macrosWithOwners)
}

func TestGetMacrosWithOwnersError(t *testing.T) {
	testServer := httptest.NewServer(http.HandlerFunc(func(res http.ResponseWriter, req *http.Request) {
		res.WriteHeader(http.StatusBadRequest)
		if _, err := res.Write([]byte("test")); err != nil {
			return
		}
	}))
	defer func() { testServer.Close() }()

	logger := &nop.Logger{}

	testClient, err := NewClient(DefaultConfig(), logger)
	require.NoError(t, err)

	testClient.config.Endpoint = testServer.URL

	_, err = testClient.GetMacrosWithOwners(context.Background())
	require.Error(t, err)
	assert.EqualError(t, err, "racktables bad response with error code '400' and body 'test'")
}

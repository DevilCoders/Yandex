package http

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/racktables"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func TestGetMacro(t *testing.T) {
	responseJSON := `{
    "ids": [
        {
            "id": "1589",
            "description": ""
        }
    ],
    "name": "_PGAASINTERNALNETS_",
    "owners": [
        "svc_hbf-agent_administration",
        "svc_internalmdb_development",
        "svc_internalmdb_hardware_management"
    ],
    "owner_service": "svc_internalmdb",
    "parent": null,
    "description": "QLOUD-934",
    "internet": 1,
    "secured": 0,
    "can_create_network": 1
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

	macro, err := testClient.GetMacro(context.Background(), "_PGAASINTERNALNETS_")
	require.NoError(t, err)

	expected := racktables.Macro{
		IDs: []racktables.MacroID{
			{
				ID:          "1589",
				Description: "",
			},
		},
		Name: "_PGAASINTERNALNETS_",
		Owners: []string{
			"svc_hbf-agent_administration",
			"svc_internalmdb_development",
			"svc_internalmdb_hardware_management",
		},
		OwnerService:     "svc_internalmdb",
		Parent:           "",
		Description:      "QLOUD-934",
		Internet:         1,
		Secured:          0,
		CanCreateNetwork: 1,
	}

	assert.Equal(t, expected, macro)
}

func TestGetMacroError(t *testing.T) {
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

	_, err = testClient.GetMacro(context.Background(), "")
	require.Error(t, err)
	assert.EqualError(t, err, "racktables bad response with error code '400' and body 'test'")
}

func TestGetMacroNotFoundError(t *testing.T) {
	testServer := httptest.NewServer(http.HandlerFunc(func(res http.ResponseWriter, req *http.Request) {
		res.WriteHeader(http.StatusInternalServerError)
		if _, err := res.Write([]byte("Record 'macro'#'_PGAASINTERNALNETS_' does not exist")); err != nil {
			return
		}
	}))
	defer func() { testServer.Close() }()

	logger := &nop.Logger{}

	testClient, err := NewClient(DefaultConfig(), logger)
	require.NoError(t, err)

	testClient.config.Endpoint = testServer.URL

	_, err = testClient.GetMacro(context.Background(), "_PGAASINTERNALNETS_")
	require.Error(t, err)
	assert.True(t, semerr.IsNotFound(err))
	assert.EqualError(t, err, "macro _PGAASINTERNALNETS_ not found")
}

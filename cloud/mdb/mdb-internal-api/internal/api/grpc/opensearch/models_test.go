package opensearch

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
)

func testDoubleConv(t *testing.T, original *osmodels.AuthProvider) {
	step1, err := AuthProviderToGRPC(original)
	require.NoError(t, err)

	res, err := AuthProviderFromGRPC(step1)
	require.NoError(t, err)
	require.Equal(t, original, res, "provider must be same after double convertions")
}

func TestAuthProviderConvertDefault(t *testing.T) {
	cases := []osmodels.AuthProviderType{
		osmodels.AuthProviderTypeNative,
		osmodels.AuthProviderTypeSaml,
	}
	for _, ptype := range cases {
		testDoubleConv(t, osmodels.NewAuthProvider(ptype, "any"))
	}
}

func TestAuthProviderConvertNoDefault(t *testing.T) {
	cases := []*osmodels.AuthProvider{
		{
			Type:        osmodels.AuthProviderTypeSaml,
			Name:        "name",
			Order:       5,
			Enabled:     true,
			Hidden:      true,
			Description: "Super Login",
			Hint:        "click me",
			Icon:        "yandex.jpg",
			Settings: &osmodels.SamlSettings{
				IDPEntityID:   "a",
				IDPMetadata:   []byte{1, 2, 3},
				SPEntityID:    "b",
				DashboardsURL: "c",
				Principal:     "d",
				Groups:        "e",
				Name:          "f",
				DN:            "g",
			},
		},
	}

	for _, original := range cases {
		testDoubleConv(t, original)
	}
}

func TestAuthProviderConvertNil(t *testing.T) {
	_, err := AuthProviderFromGRPC(nil)
	require.True(t, semerr.IsInvalidInput(err))
}

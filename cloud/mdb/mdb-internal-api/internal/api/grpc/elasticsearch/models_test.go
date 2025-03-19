package elasticsearch

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
)

func testDoubleConv(t *testing.T, original *esmodels.AuthProvider) {
	step1, err := AuthProviderToGRPC(original)
	require.NoError(t, err)

	res, err := AuthProviderFromGRPC(step1)
	require.NoError(t, err)
	require.Equal(t, original, res, "provider must be same after double convertions")
}

func TestAuthProviderConvertDefault(t *testing.T) {
	cases := []esmodels.AuthProviderType{
		esmodels.AuthProviderTypeNative,
		esmodels.AuthProviderTypeSaml,
	}
	for _, ptype := range cases {
		testDoubleConv(t, esmodels.NewAuthProvider(ptype, "any"))
	}
}

func TestAuthProviderConvertNoDefault(t *testing.T) {
	cases := []*esmodels.AuthProvider{
		{
			Type:        esmodels.AuthProviderTypeSaml,
			Name:        "name",
			Order:       5,
			Enabled:     true,
			Hidden:      true,
			Description: "Super Login",
			Hint:        "click me",
			Icon:        "yandex.jpg",
			Settings: &esmodels.SamlSettings{
				IDPEntityID: "a",
				IDPMetadata: []byte{1, 2, 3},
				SPEntityID:  "b",
				KibanaURL:   "c",
				Principal:   "d",
				Groups:      "e",
				Name:        "f",
				DN:          "g",
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

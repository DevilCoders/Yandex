package espillars

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
)

func testDoubleConv(t *testing.T, ap *esmodels.AuthProviders) {
	p := NewCluster()
	err := p.SetAuthProviders(ap)
	require.NoError(t, err)

	res, err := p.AuthProviders()
	require.NoError(t, err)

	require.Equal(t, ap, res, "providers must be same after double convertions")
}

func TestAuthProviderConvertDefault(t *testing.T) {
	ap := esmodels.NewAuthProviders()
	err := ap.Add(
		esmodels.NewTestSAMLAuthProvider("saml2"),
		esmodels.NewAuthProvider(esmodels.AuthProviderTypeNative, "native"),
		esmodels.NewTestSAMLAuthProvider("saml1"),
		// esmodels.AuthProviderTypeOpenID,
		// esmodels.AuthProviderTypeAnonymous,
	)
	require.NoError(t, err)

	testDoubleConv(t, ap)
}

func TestAuthProviderConvertNoDefault(t *testing.T) {
	ap := esmodels.NewAuthProviders()
	err := ap.Add(
		&esmodels.AuthProvider{
			Type:        esmodels.AuthProviderTypeSaml,
			Name:        "saml2",
			Order:       5,
			Enabled:     true,
			Hidden:      true,
			Description: "Super Login",
			Hint:        "click me",
			Icon:        "http://yandex.ru/logo.jpg",
			Settings:    esmodels.NewTestSAMLSettings(),
		},
		&esmodels.AuthProvider{
			Type:        esmodels.AuthProviderTypeSaml,
			Name:        "saml1",
			Order:       3,
			Enabled:     true,
			Hidden:      true,
			Description: "Other Login",
			Hint:        "do it",
			Icon:        "http://google.com/logo.jpg",
			Settings:    esmodels.NewTestSAMLSettings(),
		},
		&esmodels.AuthProvider{
			Type:        esmodels.AuthProviderTypeNative,
			Name:        "native",
			Order:       4,
			Enabled:     true,
			Description: "No way",
			Hint:        "get out",
			Icon:        "https://example.com/nothing.png",
			Settings:    &esmodels.NativeSettings{},
		},
	)
	require.NoError(t, err)

	testDoubleConv(t, ap)
}

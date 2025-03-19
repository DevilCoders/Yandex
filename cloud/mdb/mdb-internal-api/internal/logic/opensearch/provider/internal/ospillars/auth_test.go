package ospillars

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
)

func testDoubleConv(t *testing.T, ap *osmodels.AuthProviders) {
	p := NewCluster()
	err := p.SetAuthProviders(ap)
	require.NoError(t, err)

	res, err := p.AuthProviders()
	require.NoError(t, err)

	require.Equal(t, ap, res, "providers must be same after double convertions")
}

func TestAuthProviderConvertDefault(t *testing.T) {
	ap := osmodels.NewAuthProviders()
	err := ap.Add(
		osmodels.NewTestSAMLAuthProvider("saml2"),
		osmodels.NewAuthProvider(osmodels.AuthProviderTypeNative, "native"),
		osmodels.NewTestSAMLAuthProvider("saml1"),
		// osmodels.AuthProviderTypeOpenID,
		// osmodels.AuthProviderTypeAnonymous,
	)
	require.NoError(t, err)

	testDoubleConv(t, ap)
}

func TestAuthProviderConvertNoDefault(t *testing.T) {
	ap := osmodels.NewAuthProviders()
	err := ap.Add(
		&osmodels.AuthProvider{
			Type:        osmodels.AuthProviderTypeSaml,
			Name:        "saml2",
			Order:       5,
			Enabled:     true,
			Hidden:      true,
			Description: "Super Login",
			Hint:        "click me",
			Icon:        "http://yandex.ru/logo.jpg",
			Settings:    osmodels.NewTestSAMLSettings(),
		},
		&osmodels.AuthProvider{
			Type:        osmodels.AuthProviderTypeSaml,
			Name:        "saml1",
			Order:       3,
			Enabled:     true,
			Hidden:      true,
			Description: "Other Login",
			Hint:        "do it",
			Icon:        "http://google.com/logo.jpg",
			Settings:    osmodels.NewTestSAMLSettings(),
		},
		&osmodels.AuthProvider{
			Type:        osmodels.AuthProviderTypeNative,
			Name:        "native",
			Order:       4,
			Enabled:     true,
			Description: "No way",
			Hint:        "get out",
			Icon:        "https://example.com/nothing.png",
			Settings:    &osmodels.NativeSettings{},
		},
	)
	require.NoError(t, err)

	testDoubleConv(t, ap)
}

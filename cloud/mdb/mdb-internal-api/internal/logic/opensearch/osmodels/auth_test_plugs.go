package osmodels

// NewTestSAMLAuthProvider returns a plugged saml auth provider (which should only be used for tests) with a valid plugged saml settings
func NewTestSAMLAuthProvider(name string) *AuthProvider {
	p := NewAuthProvider(AuthProviderTypeSaml, name)
	p.Settings = NewTestSAMLSettings()

	return p
}

// NewTestSAMLSettings returns valid saml settings (should only be used in tests)
func NewTestSAMLSettings() *SamlSettings {
	return &SamlSettings{
		IDPEntityID:   "https://test_identity_provider.com",
		IDPMetadata:   []byte("<EntityDescriptor entityID=\"https://test_identity_provider.com\"></EntityDescriptor>"),
		SPEntityID:    "https://test_service_provider.com",
		DashboardsURL: "https://test.com",
		Principal:     "test_principal",
	}
}

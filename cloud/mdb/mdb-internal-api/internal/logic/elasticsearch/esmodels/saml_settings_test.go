package esmodels

import (
	"fmt"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestSAMLSettings_CorrectSettingsValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	p.Settings = NewTestSAMLSettings()
	err := p.Validate()
	require.NoError(t, err, "should accept a provider with a correct saml settings")
}

func TestSAMLSettings_IDPEntityValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	s := NewTestSAMLSettings()
	s.IDPEntityID = ""
	p.Settings = s
	err := p.Validate()
	require.Error(t, err, "should reject a provider with an empty IDPEntityID")

	s = NewTestSAMLSettings()
	urlPart := "test"
	s.IDPEntityID = "https://" + strings.Repeat(urlPart, (1024/len(urlPart))+1) + ".com"
	p.Settings = s
	err = p.Validate()
	require.Error(t, err, "should reject a provider with a very large IDPEntityID")
}

func TestSAMLSettings_IDPMetadataValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	s := NewTestSAMLSettings()
	s.IDPMetadata = []byte{}
	p.Settings = s
	err := p.Validate()
	require.Error(t, err, "should reject a provider with an empty IDPMetadata")

	s = NewTestSAMLSettings()
	s.IDPMetadata = []byte("<EntityDescriptor></EntityDescriptor>")
	p.Settings = s
	err = p.Validate()
	require.Error(t, err, "should reject a provider without an entityID attribute in the EntityDescriptor tag")

	s = NewTestSAMLSettings()
	s.IDPMetadata = []byte(fmt.Sprintf("<tag entityID=\"%s\"></tag>", s.IDPEntityID))
	p.Settings = s
	err = p.Validate()
	require.Error(t, err, "should reject a provider without an EntityDescriptor tag, but with entityID attribute")

	s = NewTestSAMLSettings()
	s.IDPMetadata = []byte(fmt.Sprintf("<md:EntityDescriptor entityID=\"%s\"></md:EntityDescriptor>", s.IDPEntityID))
	p.Settings = s
	err = p.Validate()
	require.NoError(t, err, "should accept a provider with an EntityDescriptor tag in some namespace")
}

func TestSAMLSettings_SPEntityValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	s := NewTestSAMLSettings()
	s.SPEntityID = ""
	p.Settings = s
	err := p.Validate()
	require.Error(t, err, "should reject a provider with an empty SPEntityID")
}

func TestSAMLSettings_KibanaURLValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	s := NewTestSAMLSettings()
	s.KibanaURL = ""
	p.Settings = s
	err := p.Validate()
	require.Error(t, err, "should reject a provider with an empty KibanaURL")

	s = NewTestSAMLSettings()
	s.KibanaURL = "http//forgot.a.slash.com"
	p.Settings = s
	err = p.Validate()
	require.Error(t, err, "should reject a provider with KibanaURL which is a bad url")

	s = NewTestSAMLSettings()
	s.KibanaURL = "not an url"
	p.Settings = s
	err = p.Validate()
	require.Error(t, err, "should reject a provider with KibanaURL which is just a simple string")
}

func TestSAMLSettings_AttributesValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeSaml, "test")

	s := NewTestSAMLSettings()
	s.Principal = ""
	p.Settings = s
	err := p.Validate()
	require.Error(t, err, "should reject a provider with an empty principal")
}

package esmodels

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestAuthProviders_AddNoOrder(t *testing.T) {
	p1 := NewAuthProvider(AuthProviderTypeNative, "native1")
	p2 := NewAuthProvider(AuthProviderTypeAnonymous, "anon1")
	p3 := NewTestSAMLAuthProvider("saml1")
	p4 := NewAuthProvider(AuthProviderTypeOpenID, "openid1")
	ap := NewAuthProviders()

	err := ap.Add(p1, p2, p3, p4)

	require.NoError(t, err, "should successfully add each provider one time")
	require.Len(t, ap.Providers(), 4, "should add all 4 providers")
	require.Equal(t, ap.Providers(), []*AuthProvider{p1, p2, p3, p4}, "should add providers in order")
}

func TestAuthProviders_AddInverseOrder(t *testing.T) {
	p1 := NewAuthProvider(AuthProviderTypeNative, "native1")
	p1.Order = 4
	p2 := NewAuthProvider(AuthProviderTypeAnonymous, "anon1")
	p2.Order = 3
	p3 := NewTestSAMLAuthProvider("saml1")
	p3.Order = 2
	p4 := NewAuthProvider(AuthProviderTypeOpenID, "openid1")
	p4.Order = 1
	ap := NewAuthProviders()

	err := ap.Add(p1, p2, p3, p4)

	require.NoError(t, err, "should successfully add each provider one time")
	require.Len(t, ap.Providers(), 4, "should add all 4 providers")
	require.Equal(t, ap.Providers(), []*AuthProvider{p4, p3, p2, p1}, "should add providers in right order")
}

func TestAuthProviders_AddZeroOrderLast(t *testing.T) {
	p1 := NewAuthProvider(AuthProviderTypeNative, "native1")
	p1.Order = 4
	p2 := NewAuthProvider(AuthProviderTypeAnonymous, "anon1")
	p2.Order = 0
	p3 := NewTestSAMLAuthProvider("saml1")
	p3.Order = 2
	p4 := NewAuthProvider(AuthProviderTypeOpenID, "openid1")
	p4.Order = 1
	ap := NewAuthProviders()

	err := ap.Add(p1, p2, p3, p4)

	require.NoError(t, err, "should successfully add each provider one time")
	require.Len(t, ap.Providers(), 4, "should add all 4 providers")
	require.Equal(t, ap.Providers(), []*AuthProvider{p4, p3, p1, p2}, "should add providers in right order")
}

func TestAuthProviders_AddZeroOrderx(t *testing.T) {
	p1 := NewAuthProvider(AuthProviderTypeNative, "native1")
	p1.Order = 1
	p2 := NewAuthProvider(AuthProviderTypeAnonymous, "saml1")
	p2.Order = 0
	ap := NewAuthProviders()

	err := ap.Add(p1, p2)

	require.NoError(t, err, "should successfully add each provider one time")
	require.Len(t, ap.Providers(), 2, "should add all 2 providers")
	require.Equal(t, ap.Providers(), []*AuthProvider{p1, p2}, "should add providers in right order")
}

func TestAuthProviders_AddUniqueType(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(
		NewAuthProvider(AuthProviderTypeNative, "native1"),
		NewAuthProvider(AuthProviderTypeAnonymous, "anon1"),
		NewTestSAMLAuthProvider("saml1"),
		NewAuthProvider(AuthProviderTypeOpenID, "openid1"),
	)

	require.NoError(t, err, "should successfully add each provider one time")

	err = ap.Add(
		NewTestSAMLAuthProvider("saml2"),
		NewAuthProvider(AuthProviderTypeOpenID, "openid2"),
	)

	require.NoError(t, err, "should successfully add more not unique providers")

	err = ap.Add(
		NewAuthProvider(AuthProviderTypeNative, "native2"),
	)

	require.Error(t, err, "should not add unique provider more than once")

	err = ap.Add(
		NewAuthProvider(AuthProviderTypeAnonymous, "anon2"),
	)

	require.Error(t, err, "should not add unique provider more than once")
}

func TestAuthProviders_AddEmptyName(t *testing.T) {
	err := NewAuthProviders().Add(
		NewAuthProvider(AuthProviderTypeNative, ""),
	)

	require.Error(t, err, "should not add provider with empty name")
}

func TestAuthProviders_AddSameName(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(NewTestSAMLAuthProvider("samename"))
	require.NoError(t, err, "should successfully add one provider")

	err = ap.Add(NewAuthProvider(AuthProviderTypeOpenID, "samename"))
	require.Error(t, err, "should not add provider with already used name")
}

func TestAuthProviders_AddMax(t *testing.T) {
	ap := NewAuthProviders()
	for i := 0; i < MaxAuthProviders; i++ {
		err := ap.Add(NewTestSAMLAuthProvider(fmt.Sprintf("name%d", i)))
		require.NoError(t, err, "should successfully provider")
	}

	err := ap.Add(NewAuthProvider(AuthProviderTypeOpenID, "name100"))
	require.Error(t, err, "should not add provider when max added")
}

func names(ap *AuthProviders) []string {
	var res = make([]string, 0, 16)
	ps := ap.Providers()
	for i := range ps {
		res = append(res, ps[i].Name)
	}
	return res
}

func TestAuthProviders_AddKeepOrdering(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(NewTestSAMLAuthProvider("first"))
	require.NoError(t, err, "should successfully add provider")

	p1 := NewTestSAMLAuthProvider("new1").WithOrder(50)
	err = ap.Add(p1)
	require.NoError(t, err, "should successfully add provider")
	require.Equal(t, []string{"first", "new1"}, names(ap), "should successfully add provider with big order as last")

	p2 := NewTestSAMLAuthProvider("new2").WithOrder(0)
	err = ap.Add(p2)
	require.NoError(t, err, "should successfully add provider")
	require.Equal(t, []string{"first", "new1", "new2"}, names(ap), "should successfully add provider with 0 order as last")

	p3 := NewTestSAMLAuthProvider("new3").WithOrder(2)
	err = ap.Add(p3)
	require.NoError(t, err, "should successfully add provider")
	require.Equal(t, []string{"first", "new3", "new1", "new2"}, names(ap), "should successfully add provider to with specified position")

	ps := ap.Providers()
	for i := range ps {
		require.Equal(t, i+1, ps[i].Order, "providers must be reordered accordingly")
	}

}

func TestAuthProviders_DeleteKeepOrdering(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(
		NewTestSAMLAuthProvider("first"),
		NewTestSAMLAuthProvider("second"),
		NewTestSAMLAuthProvider("third"),
	)
	require.NoError(t, err, "should successfully add providers")

	err = ap.Delete("second")
	require.NoError(t, err, "should successfully delete provider")
	require.Equal(t, []string{"first", "third"}, names(ap), "providers must be reordered accordingly")

	ps := ap.Providers()
	for i := range ps {
		require.Equal(t, i+1, ps[i].Order, "providers must be reordered accordingly")
	}
}

func TestAuthProviders_UpdateSamePos(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(
		NewTestSAMLAuthProvider("first"),
		NewTestSAMLAuthProvider("second"),
		NewTestSAMLAuthProvider("third"),
	)
	require.NoError(t, err, "should successfully add providers")

	err = ap.Update("second", NewTestSAMLAuthProvider("forth").WithOrder(0))
	require.NoError(t, err, "should successfully update provider")
	require.Equal(t, []string{"first", "forth", "third"}, names(ap), "providers must not be reordered accordingly")

	ps := ap.Providers()
	for i := range ps {
		require.Equal(t, i+1, ps[i].Order, "providers must be reordered accordingly")
	}
}

func TestAuthProviders_UpdateOtherPos(t *testing.T) {
	ap := NewAuthProviders()
	err := ap.Add(
		NewTestSAMLAuthProvider("first"),
		NewTestSAMLAuthProvider("second"),
		NewTestSAMLAuthProvider("third"),
	)
	require.NoError(t, err, "should successfully add providers")

	err = ap.Update("second", NewTestSAMLAuthProvider("second").WithOrder(1))
	require.NoError(t, err, "should successfully update provider")
	require.Equal(t, []string{"second", "first", "third"}, names(ap), "providers must not be reordered accordingly")

	ps := ap.Providers()
	for i := range ps {
		require.Equal(t, i+1, ps[i].Order, "providers must be reordered accordingly")
	}
}

func TestAuthProviders_DescValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeNative, "first")
	p.Description = "#$\n"
	err := p.Validate()
	require.Error(t, err, "should error on special symbols in description")
}

func TestAuthProviders_HintValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeNative, "first")
	p.Hint = "#$\n"
	err := p.Validate()
	require.Error(t, err, "should error on special symbols in hint")
}

func TestAuthProviders_IconValidation(t *testing.T) {
	p := NewAuthProvider(AuthProviderTypeNative, "first")
	p.Icon = "file://bad_url"
	err := p.Validate()
	require.Error(t, err, "should error on not valid icon url")
}

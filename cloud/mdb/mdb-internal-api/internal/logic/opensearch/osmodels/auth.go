package osmodels

import (
	"fmt"
	"net/url"
	"regexp"
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/internal/valid"
)

type AuthProviderType string

const (
	AuthProviderTypeNative    AuthProviderType = "native"
	AuthProviderTypeSaml      AuthProviderType = "saml"
	AuthProviderTypeOpenID    AuthProviderType = "openid"
	AuthProviderTypeAnonymous AuthProviderType = "anonymous"
)

func (t AuthProviderType) String() string {
	return string(t)
}

func (t AuthProviderType) Unique() bool {
	return t == AuthProviderTypeNative || t == AuthProviderTypeAnonymous
}

// NativeKibanaProvider returns true if type don't have own kibana provider otherwise false
func (t AuthProviderType) NativeKibanaProvider() bool {
	return t == AuthProviderTypeNative
}

// AuthProvider concrete auth provider instance with all settings
type AuthProvider struct {
	Type    AuthProviderType
	Name    string
	Order   int
	Enabled bool

	Hidden      bool
	Description string
	Hint        string
	Icon        string

	Settings AuthProviderSettings
}

type AuthProviderSettings interface{}

func NewAuthProvider(t AuthProviderType, name string) *AuthProvider {
	return &AuthProvider{
		Type:     t,
		Name:     name,
		Enabled:  true,
		Settings: makeSettings(t),
	}
}

func makeSettings(t AuthProviderType) AuthProviderSettings {
	switch t {
	case AuthProviderTypeNative:
		return &NativeSettings{}
	case AuthProviderTypeSaml:
		return &SamlSettings{}
	case AuthProviderTypeOpenID:
		return &OpenIDSettings{}
	case AuthProviderTypeAnonymous:
		return &AnonymousSettings{}
	}
	panic(fmt.Sprintf("unknown auth provider type: %q", t))
}

func (p *AuthProvider) WithOrder(order int) *AuthProvider {
	p.Order = order
	return p
}

var namePattern = regexp.MustCompile("[a-z][a-z0-9_-]*")

func (p *AuthProvider) Validate() error {
	switch p.Type {
	case AuthProviderTypeNative,
		AuthProviderTypeSaml,
		AuthProviderTypeOpenID,
		AuthProviderTypeAnonymous:
	default:
		return semerr.InvalidInputf("invalid provider type %q", p.Type)
	}

	if !namePattern.MatchString(p.Name) {
		return semerr.InvalidInputf("invalid provider name %q, must match %s", p.Name, namePattern)
	}

	switch p.Type {
	case AuthProviderTypeNative:
		if p.Hidden {
			return semerr.InvalidInputf("native provider can't be hidden")
		}
	case AuthProviderTypeSaml:
		ss, ok := p.Settings.(*SamlSettings)
		if !ok {
			return semerr.Internal("saml provider settings have a bad type")
		}
		if err := ss.validate(); err != nil {
			return semerr.InvalidInputf("saml provider settings are invalid: %s", err.Error())
		}
	}

	if p.Icon != "" && p.Icon != "logoElasticsearch" {
		u, err := url.Parse(p.Icon)
		if err != nil {
			return semerr.InvalidInputf("can't parse provider %q icon url: %s", p.Name, err.Error())
		}
		// http, https or relative
		if u.Scheme != "http" && u.Scheme != "https" && u.Scheme != "" {
			return semerr.InvalidInputf("can't parse provider %q icon url: invalid scheme", p.Name)
		}
	}

	if err := AuthProviderDescriptionValidator.ValidateString(p.Description); err != nil {
		return err
	}

	if err := AuthProviderHintValidator.ValidateString(p.Hint); err != nil {
		return err
	}

	return nil
}

type NativeSettings struct{}

type OpenIDSettings struct{}

type AnonymousSettings struct{}

const MaxAuthProviders = 20

// AuthProviders represents list of auth providers and checks all required invariants on updates
type AuthProviders struct {
	providers []*AuthProvider
}

func NewAuthProviders() *AuthProviders {
	return &AuthProviders{make([]*AuthProvider, 0, MaxAuthProviders)}
}

func (ap *AuthProviders) find(name string) (int, bool) {
	for i := range ap.providers {
		if ap.providers[i].Name == name {
			return i, true
		}
	}
	return 0, false
}

func (ap *AuthProviders) findByType(t AuthProviderType) (int, bool) {
	for i := range ap.providers {
		if ap.providers[i].Type == t {
			return i, true
		}
	}
	return 0, false
}

// update orders
func (ap *AuthProviders) reorder() {
	for i := range ap.providers {
		ap.providers[i].Order = i + 1
	}
}

func (ap *AuthProviders) delete(name string) error {
	if idx, ok := ap.find(name); ok {
		ps := ap.providers
		ap.providers = append(ps[:idx], ps[idx+1:]...)
		ap.reorder()
		return nil
	}

	return semerr.NotFoundf("auth provider %q not found", name)
}

func (ap *AuthProviders) add(p *AuthProvider) error {
	if len(ap.providers) >= MaxAuthProviders {
		return semerr.FailedPreconditionf("too many auth providers, remove some to add more")
	}

	if err := p.Validate(); err != nil {
		return err
	}

	if _, ok := ap.find(p.Name); ok {
		return semerr.AlreadyExistsf("auth provider %q already exists", p.Name)
	}

	if p.Type.Unique() {
		if _, ok := ap.findByType(p.Type); ok {
			return semerr.AlreadyExistsf("multiple providers of %q type not allowed", p.Type)
		}
	}

	if 0 < p.Order && p.Order <= len(ap.providers) {
		ps := ap.providers
		ap.providers = append(ps[:p.Order], ps[p.Order-1:]...)
		ap.providers[p.Order-1] = p
	} else {
		ap.providers = append(ap.providers, p)
	}

	ap.reorder()
	return nil
}

func (ap *AuthProviders) Delete(names ...string) error {
	for i := range names {
		if err := ap.delete(names[i]); err != nil {
			return err
		}
	}
	return nil
}

func (ap *AuthProviders) Add(providers ...*AuthProvider) error {
	sort.SliceStable(providers, func(i, j int) bool {
		return providers[i].Order != 0 && (providers[j].Order == 0 || providers[i].Order < providers[j].Order)
	})

	for i := range providers {
		if err := ap.add(providers[i]); err != nil {
			return err
		}
	}
	return nil
}

func (ap *AuthProviders) Find(name string) (*AuthProvider, error) {
	if idx, ok := ap.find(name); ok {
		return ap.providers[idx], nil
	}

	return nil, semerr.NotFoundf("auth provider %q not found", name)
}

func (ap *AuthProviders) Update(name string, p *AuthProvider) error {
	if p.Order <= 0 {
		if idx, ok := ap.find(name); ok {
			p.Order = ap.providers[idx].Order
		}
	}

	if err := ap.delete(name); err != nil {
		return err
	}

	return ap.add(p)
}

func (ap *AuthProviders) Providers() []*AuthProvider {
	return ap.providers
}

const (
	AuthProviderDescriptionPattern = "^[a-zA-Z0-9_,./() -]*$"
	AuthProviderDescriptionMinLen  = 0
	AuthProviderDescriptionMaxLen  = 256
)

var AuthProviderDescriptionValidator = valid.MustStringComposedValidator(
	&valid.Regexp{
		Pattern: AuthProviderDescriptionPattern,
		Msg:     "auth provider description %q has invalid symbols",
	},
	&valid.StringLength{
		Min:         AuthProviderDescriptionMinLen,
		Max:         AuthProviderDescriptionMaxLen,
		TooShortMsg: "auth provider description %q is too short",
		TooLongMsg:  "auth provider description %.64q is too long",
	},
)

const (
	AuthProviderHintPattern = "^[a-zA-Z0-9_,./() -]*$"
	AuthProviderHintMinLen  = 0
	AuthProviderHintMaxLen  = 256
)

var AuthProviderHintValidator = valid.MustStringComposedValidator(
	&valid.Regexp{
		Pattern: AuthProviderHintPattern,
		Msg:     "auth provider hint %q has invalid symbols",
	},
	&valid.StringLength{
		Min:         AuthProviderHintMinLen,
		Max:         AuthProviderHintMaxLen,
		TooShortMsg: "auth provider hint %q is too short",
		TooLongMsg:  "auth provider hint %.64q is too long",
	},
)

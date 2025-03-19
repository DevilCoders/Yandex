package espillars

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Auth struct {
	Providers []KibanaProvider `json:"providers,omitempty"`
	Realms    struct {
		Native []NativeRealm `json:"native,omitempty"`
		Saml   []SamlRealm   `json:"saml,omitempty"`
	} `json:"realms"`
}

type KibanaProvider struct {
	Type     string `json:"type"`
	Name     string `json:"name"`
	Settings struct {
		Order   int  `json:"order"`
		Enabled bool `json:"enabled"`

		Description    string `json:"description,omitempty"`
		Hint           string `json:"hint,omitempty"`
		Icon           string `json:"icon,omitempty"`
		ShowInSelector bool   `json:"showInSelector"`
		// for saml and openid
		Realm string `json:"realm,omitempty"`
	} `json:"settings"`
}

type NativeRealm struct {
	Name     string `json:"name"`
	Settings struct {
		Order   int  `json:"order"`
		Enabled bool `json:"enabled"`
	} `json:"settings"`
}

type SamlRealm struct {
	Name     string `json:"name"`
	Settings struct {
		Order           int    `json:"order"`
		Enabled         bool   `json:"enabled"`
		IDPEntityID     string `json:"idp.entity_id"`
		IDPMetadataPath string `json:"idp.metadata.path"`
		SPEntityID      string `json:"sp.entity_id"`
		SPAcs           string `json:"sp.acs"`
		SPLogout        string `json:"sp.logout,omitempty"`
		AttrPrincipal   string `json:"attributes.principal,omitempty"`
		AttrGroups      string `json:"attributes.groups,omitempty"`
		AttrName        string `json:"attributes.name,omitempty"`
		AttrMail        string `json:"attributes.mail,omitempty"`
		AttrDN          string `json:"attributes.dn,omitempty"`
	} `json:"settings"`
	Files struct {
		Metadata []byte `json:"metadata,omitempty"`
	} `json:"files"`
}

const (
	KibanaNativeProviderName      = "Native"
	KibanaNativeProviderTypeBasic = "basic"
	KibanaNativeProviderTypeToken = "token"
)

func (a *Auth) authProviders() (*esmodels.AuthProviders, error) {
	var res = make([]*esmodels.AuthProvider, 0, esmodels.MaxAuthProviders)

	for _, realm := range a.Realms.Native {
		p := esmodels.NewAuthProvider(esmodels.AuthProviderTypeNative, realm.Name)
		p.Order = realm.Settings.Order
		p.Enabled = realm.Settings.Enabled

		kp, err := a.providerNative()
		if err != nil {
			return nil, err
		}
		a.fromProvider(kp, p)

		res = append(res, p)
	}

	for _, r := range a.Realms.Saml {
		p := esmodels.NewAuthProvider(esmodels.AuthProviderTypeSaml, r.Name)
		p.Order = r.Settings.Order
		p.Enabled = r.Settings.Enabled

		kp, err := a.providerByName(r.Name)
		if err != nil {
			return nil, err
		}
		a.fromProvider(kp, p)

		s := p.Settings.(*esmodels.SamlSettings)
		s.IDPMetadata = r.Files.Metadata

		s.IDPEntityID = r.Settings.IDPEntityID
		s.SPEntityID = r.Settings.SPEntityID
		s.KibanaURL = strings.TrimSuffix(r.Settings.SPLogout, "/logout")
		s.Principal = r.Settings.AttrPrincipal
		s.Groups = r.Settings.AttrGroups
		s.Name = r.Settings.AttrName
		s.Mail = r.Settings.AttrMail
		s.DN = r.Settings.AttrDN

		res = append(res, p)
	}

	ap := esmodels.NewAuthProviders()
	if err := ap.Add(res...); err != nil {
		return nil, err
	}
	return ap, nil
}

func (a *Auth) providerByName(name string) (KibanaProvider, error) {
	for i := range a.Providers {
		if a.Providers[i].Name == name {
			return a.Providers[i], nil
		}
	}
	return KibanaProvider{}, xerrors.New("kibana provider not found " + name)
}

func (a *Auth) providerNative() (KibanaProvider, error) {
	for i := range a.Providers {
		if a.Providers[i].Name == KibanaNativeProviderName {
			return a.Providers[i], nil
		}
	}
	return KibanaProvider{}, xerrors.New("native kibana provider not found")
}

func (a *Auth) fromProvider(kp KibanaProvider, p *esmodels.AuthProvider) {
	p.Description = kp.Settings.Description
	p.Hint = kp.Settings.Hint
	p.Icon = kp.Settings.Icon
	p.Hidden = !kp.Settings.ShowInSelector
}

func (a *Auth) clear() {
	a.Providers = a.Providers[:0]
	a.Realms.Native = a.Realms.Native[:0]
	a.Realms.Saml = a.Realms.Saml[:0]
}

func (a *Auth) addProvider(p *esmodels.AuthProvider) {
	var kp KibanaProvider
	kp.Name = p.Name
	kp.Type = p.Type.String()
	kp.Settings.Order = p.Order
	kp.Settings.Enabled = p.Enabled

	kp.Settings.Description = p.Description
	kp.Settings.Hint = p.Hint
	kp.Settings.Icon = p.Icon
	kp.Settings.ShowInSelector = !p.Hidden

	if p.Type == esmodels.AuthProviderTypeSaml {
		kp.Settings.Realm = p.Name
	}

	a.Providers = append(a.Providers, kp)
}

func (a *Auth) addNativeProvider(p *esmodels.AuthProvider, edition esmodels.Edition) {
	var kp KibanaProvider
	kp.Name = KibanaNativeProviderName
	kp.Type = KibanaNativeProviderTypeBasic
	if edition.Includes(esmodels.EditionGold) {
		kp.Type = KibanaNativeProviderTypeToken
	}
	kp.Settings.Order = p.Order
	kp.Settings.Enabled = true

	kp.Settings.Description = p.Description
	kp.Settings.Hint = p.Hint
	kp.Settings.Icon = p.Icon
	kp.Settings.ShowInSelector = true

	a.Providers = append(a.Providers, kp)
}

func (a *Auth) setAuthProviders(ap *esmodels.AuthProviders, edition esmodels.Edition) error {
	a.clear()

	var nativeProviderInited bool
	for _, p := range ap.Providers() {
		// add kibana provider
		if p.Type.NativeKibanaProvider() {
			if !nativeProviderInited {
				a.addNativeProvider(p, edition)
				nativeProviderInited = true
			}
		} else {
			a.addProvider(p)
		}

		// add elastic realm
		switch p.Type {
		case esmodels.AuthProviderTypeNative:
			var r NativeRealm
			r.Name = p.Name
			r.Settings.Order = p.Order
			r.Settings.Enabled = p.Enabled

			a.Realms.Native = append(a.Realms.Native, r)

		case esmodels.AuthProviderTypeSaml:
			var r SamlRealm
			r.Name = p.Name
			r.Settings.Order = p.Order
			r.Settings.Enabled = p.Enabled

			s := p.Settings.(*esmodels.SamlSettings)
			r.Settings.IDPEntityID = s.IDPEntityID
			r.Settings.IDPMetadataPath = "auth_files/" + p.Name + "_metadata"
			r.Settings.SPEntityID = s.SPEntityID
			r.Settings.SPAcs = strings.TrimSuffix(s.KibanaURL, "/") + "/api/security/saml/callback"
			r.Settings.SPLogout = strings.TrimSuffix(s.KibanaURL, "/") + "/logout"
			r.Settings.AttrPrincipal = s.Principal
			r.Settings.AttrGroups = s.Groups
			r.Settings.AttrName = s.Name
			r.Settings.AttrMail = s.Mail
			r.Settings.AttrDN = s.DN

			r.Files.Metadata = s.IDPMetadata

			a.Realms.Saml = append(a.Realms.Saml, r)
		}
	}

	return nil
}

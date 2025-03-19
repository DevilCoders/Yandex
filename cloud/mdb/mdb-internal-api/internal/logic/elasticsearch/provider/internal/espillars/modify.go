package espillars

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/elasticsearch/esmodels"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type PillarUpdater struct {
	pillar     *Cluster
	hasChanges bool
}

func NewPillarUpdater(p *Cluster) *PillarUpdater {
	return &PillarUpdater{pillar: p}
}

func (u *PillarUpdater) Pillar() *Cluster {
	return u.pillar
}

func (u *PillarUpdater) HasChanges() bool {
	return u.hasChanges
}

func (u *PillarUpdater) UpdateVersion(v esmodels.OptionalVersion) error {
	if !v.Valid {
		return nil
	}
	if u.pillar.Version().Equal(v.Value) {
		return nil
	}
	if u.pillar.Version().Greater(v.Value) {
		return semerr.FailedPrecondition("version downgrade is not allowed")
	}
	u.pillar.Data.ElasticSearch.Version = v.Value
	u.hasChanges = true
	return nil
}

func (u *PillarUpdater) UpdateEdition(e esmodels.OptionalEdition, allowed []esmodels.Edition) error {
	if !e.Valid {
		return nil
	}

	if err := esmodels.CheckAllowedEdition(e.Value, allowed); err != nil {
		return err
	}

	if e.Value == u.pillar.Edition() {
		return nil
	}

	u.pillar.Data.ElasticSearch.Edition = e.Value
	u.hasChanges = true
	return nil // nothing to update
}

func (u *PillarUpdater) UpdateAdminPassword(password esmodels.OptionalSecretString, cryptoProvider crypto.Crypto) error {
	if !password.Valid {
		return nil
	}

	if err := esmodels.ValidateAdminPassword(password.Value); err != nil {
		return err
	}

	user, ok := u.pillar.GetUser(esmodels.UserAdmin)
	if !ok { // old cluster without admin user
		user = UserData{
			Name:     esmodels.UserAdmin,
			Internal: false,
		}
	}
	passwordEncrypted, err := cryptoProvider.Encrypt([]byte(password.Value.Unmask()))
	if err != nil {
		return xerrors.Errorf("encrypt user %q password: %w", user.Name, err)
	}
	user.Password = passwordEncrypted
	u.pillar.SetUser(user)
	u.hasChanges = true
	return nil
}

func (u *PillarUpdater) UpdateServiceAccountID(id optional.String) error {
	if !id.Valid {
		return nil
	}

	if id.String != u.pillar.Data.ServiceAccountID {
		u.pillar.Data.ServiceAccountID = id.String
		u.hasChanges = true
	}
	return nil
}

func (u *PillarUpdater) UpdatePlugins(plugins esmodels.OptionalPlugins) error {
	if !plugins.Valid {
		return nil
	}

	if err := u.pillar.SetPlugins(plugins.Value...); err != nil {
		return err
	}
	u.hasChanges = true
	return nil
}

func (u *PillarUpdater) UpdateAccess(access esmodels.OptionalAccess) {
	if !access.Valid {
		return
	}
	u.pillar.SetAccess(access)
	u.hasChanges = true
}

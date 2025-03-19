package ospillars

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/opensearch/osmodels"
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

func (u *PillarUpdater) UpdateVersion(v osmodels.OptionalVersion) error {
	if !v.Valid {
		return nil
	}
	if u.pillar.Version().Equal(v.Value) {
		return nil
	}
	if u.pillar.Version().Greater(v.Value) {
		return semerr.FailedPrecondition("version downgrade is not allowed")
	}
	u.pillar.Data.OpenSearch.Version = v.Value
	u.hasChanges = true
	return nil
}

func (u *PillarUpdater) UpdateAdminPassword(password osmodels.OptionalSecretString, cryptoProvider crypto.Crypto) error {
	if !password.Valid {
		return nil
	}

	if err := osmodels.ValidateAdminPassword(password.Value); err != nil {
		return err
	}

	user, ok := u.pillar.GetUser(osmodels.UserAdmin)
	if !ok { // old cluster without admin user
		user = UserData{
			Name:     osmodels.UserAdmin,
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

func (u *PillarUpdater) UpdatePlugins(plugins osmodels.OptionalPlugins) error {
	if !plugins.Valid {
		return nil
	}

	if err := u.pillar.SetPlugins(plugins.Value...); err != nil {
		return err
	}
	u.hasChanges = true
	return nil
}

func (u *PillarUpdater) UpdateAccess(access osmodels.OptionalAccess) {
	if !access.Valid {
		return
	}
	u.pillar.SetAccess(access)
	u.hasChanges = true
}

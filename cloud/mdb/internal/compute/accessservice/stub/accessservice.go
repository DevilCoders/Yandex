package stub

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

// AccessService mock for client
type AccessService struct {
	Mapping map[string]Data
}

var _ as.AccessService = &AccessService{}

type PermissionSet struct {
	ReadOnly    bool
	ForceDelete bool
	ReadSecrets bool
}

type Data struct {
	Subject         as.Subject
	Folders         map[string]PermissionSet
	Clouds          map[string]struct{}
	ServiceAccounts map[string]struct{}
}

func (a *AccessService) Auth(ctx context.Context, token string, permission string, resources ...as.Resource) (as.Subject, error) {
	d, ok := a.Mapping[token]
	if !ok {
		return as.Subject{}, semerr.Authentication("authentication failed")
	}
	err := a.checkPermission(d, permission, resources...)
	if err != nil {
		return as.Subject{}, err
	}

	return d.Subject, nil
}

func (a *AccessService) Authorize(_ context.Context, subject cloudauth.Subject, permission string, resources ...as.Resource) error {
	var subjectID = ""
	switch s := subject.(type) {
	case cloudauth.AnonymousAccount:
		return semerr.Authorization("anonymous account")
	case cloudauth.UserAccount:
		subjectID = s.ID
	case cloudauth.ServiceAccount:
		subjectID = s.ID
	}

	for _, data := range a.Mapping {
		if data.Subject.MustID() == subjectID {
			return a.checkPermission(data, permission, resources...)
		}
	}
	return semerr.Authentication("authentication failed")
}

func (a *AccessService) checkPermission(data Data, permission string, resources ...as.Resource) error {
	for _, res := range resources {
		switch res.Type {
		case as.ResourceTypeCloud:
			if _, ok := data.Clouds[res.ID]; !ok {
				return semerr.Authorization("authorization failed")
			}
		case as.ResourceTypeFolder:
			permissionset, ok := data.Folders[res.ID]
			if !ok {
				return semerr.Authorization("authorization failed")
			}
			isReadOnly := strings.HasSuffix(permission, ".all.read") || strings.HasSuffix(permission, ".clusters.get")
			if permissionset.ReadOnly && !isReadOnly {
				return semerr.Authorization("authorization failed")
			}
			isForceDelete := strings.HasSuffix(permission, ".all.forceDelete") || strings.HasSuffix(permission, ".clusters.forceDelete")
			if !permissionset.ForceDelete && isForceDelete {
				return semerr.Authorization("authorization failed")
			}
			if !permissionset.ReadSecrets && permission == "mdb.clusters.getSecret" {
				return semerr.Authorization("authorization failed")
			}
		case as.ResourceTypeServiceAccount:
			if permission != "iam.serviceAccounts.use" {
				return semerr.InvalidInputf("authorization failed")
			}

			if _, ok := data.ServiceAccounts[res.ID]; !ok {
				return semerr.InvalidInputf("authorization failed")
			}

			return nil
		}
	}

	return nil // All Ok
}

package functest

import (
	as "a.yandex-team.ru/cloud/mdb/internal/compute/accessservice"
	"a.yandex-team.ru/cloud/mdb/internal/compute/accessservice/stub"
)

func AccessServiceStub() as.AccessService {
	return &stub.AccessService{
		Mapping: map[string]stub.Data{
			"ro-token": {
				Subject:         as.Subject{User: &as.UserAccount{ID: "read-only-user"}},
				Folders:         map[string]stub.PermissionSet{"folder1": {ReadOnly: true}},
				Clouds:          map[string]struct{}{"cloud1": {}},
				ServiceAccounts: map[string]struct{}{},
			},
			"rw-token": {
				Subject:         as.Subject{User: &as.UserAccount{ID: "user"}},
				Folders:         map[string]stub.PermissionSet{"folder1": {ReadSecrets: true}, "folder2": {}, "folder3": {}, "folder4": {}},
				Clouds:          map[string]struct{}{"cloud1": {}, "cloud2": {}, "cloud3": {}},
				ServiceAccounts: map[string]struct{}{"sa1": {}, "service_account_1": {}},
			},
			"rw-service-token": {
				Subject:         as.Subject{Service: &as.ServiceAccount{ID: "service-user"}},
				Folders:         map[string]stub.PermissionSet{"folder1": {}},
				Clouds:          map[string]struct{}{"cloud1": {}},
				ServiceAccounts: map[string]struct{}{},
			},
			"resource-reaper-service-token": {
				Subject:         as.Subject{Service: &as.ServiceAccount{ID: "resource-reaper"}},
				Folders:         map[string]stub.PermissionSet{"folder1": {ForceDelete: true}},
				Clouds:          map[string]struct{}{"cloud1": {}},
				ServiceAccounts: map[string]struct{}{},
			},
			"rw-e2e-token": {
				Subject:         as.Subject{User: &as.UserAccount{ID: "e2e-user"}},
				Folders:         map[string]stub.PermissionSet{"folder3": {}},
				Clouds:          map[string]struct{}{"cloud3": {}},
				ServiceAccounts: map[string]struct{}{},
			},
		},
	}
}

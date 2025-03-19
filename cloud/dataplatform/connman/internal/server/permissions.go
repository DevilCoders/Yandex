package server

import (
	"a.yandex-team.ru/cloud/iam/accessservice/client/go/cloudauth"
	"a.yandex-team.ru/transfer_manager/go/pkg/auth"
)

const (
	PermissionConnectionGet           = auth.Permission("connman.connection.get")
	PermissionConnectionViewSensitive = auth.Permission("connman.connection.view-sensitive")
	PermissionConnectionCreate        = auth.Permission("connman.connection.create")
	PermissionConnectionUpdate        = auth.Permission("connman.connection.update")
	PermissionConnectionDelete        = auth.Permission("connman.connection.delete")
)

func ResourceConnection(id string) cloudauth.Resource {
	return cloudauth.Resource{
		ID:   id,
		Type: "connman.connection",
	}
}

package osmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var SystemUsers = []string{"mdb_admin", "mdb_monitor", "mdb_kibana"}

const UserAdmin = "admin"
const RoleAdmin = "superuser"

type User struct {
	ClusterID string
	Name      string
}

var userPasswordValidator = models.MustUserPasswordValidator(models.DefaultUserPasswordPattern)

var adminPasswordValidator = models.MustUserPasswordValidator("^[ -~]+$") // ASCII charset quick fix for MDB-12803 catch it early

func ValidateAdminPassword(password secret.String) error {
	return adminPasswordValidator.ValidateString(password.Unmask())
}

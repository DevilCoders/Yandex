package esmodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

var SystemUsers = []string{"mdb_admin", "mdb_monitor", "mdb_kibana"}

const UserAdmin = "admin"
const RoleAdmin = "superuser"

var BlacklistedUsers = []string{
	"elastic", "kibana", "apm_system",
	"beats_system", "kibana_system", "logstash_system",
	"remote_monitoring_user", UserAdmin,
}

type User struct {
	ClusterID string
	Name      string
}

type UserSpec struct {
	Name     string
	Password secret.String
}

var UserNameValidator = models.MustUserNameValidator(models.DefaultUserNamePattern, BlacklistedUsers)
var userPasswordValidator = models.MustUserPasswordValidator(models.DefaultUserPasswordPattern)

func (us UserSpec) Validate() error {
	if err := UserNameValidator.ValidateString(us.Name); err != nil {
		return err
	}
	if err := userPasswordValidator.ValidateString(us.Password.Unmask()); err != nil {
		return err
	}
	return nil
}

var adminPasswordValidator = models.MustUserPasswordValidator("^[ -~]+$") // ASCII charset quick fix for MDB-12803 catch it early

func ValidateAdminPassword(password secret.String) error {
	return adminPasswordValidator.ValidateString(password.Unmask())
}

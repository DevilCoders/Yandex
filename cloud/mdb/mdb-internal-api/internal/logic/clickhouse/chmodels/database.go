package chmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
)

type Database struct {
	ClusterID string
	Name      string
}

type DatabaseSpec struct {
	Name string
}

func (ds DatabaseSpec) Validate() error {
	if err := validateDatabaseName(ds.Name); err != nil {
		return err
	}

	return nil
}

var databaseNameValidator = models.MustDatabaseNameValidator(models.DefaultDatabaseNamePattern, []string{"default", "system", "_system"})

func validateDatabaseName(name string) error {
	return databaseNameValidator.ValidateString(name)
}

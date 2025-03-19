package mongomodels

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/slices"
)

var (
	SystemDatabases = []string{"admin", "local", "config", "mdb_internal"}
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

func validateDatabaseName(name string) error {
	if isSystemDatabase(name) {
		return semerr.InvalidInputf("invalid database name %q", name)
	}

	// TODO: validate via regexp and string len
	// TODO: move validation params to traits
	return nil
}

func isSystemDatabase(name string) bool {
	return slices.ContainsString(SystemDatabases, name)
}

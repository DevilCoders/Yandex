package migrator

import (
	"strconv"

	"a.yandex-team.ru/cloud/mdb/tools/pgmigrator/internal/pkg/pgmigrate"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func plan(l log.Logger, migrations []pgmigrate.Migration, target int) []pgmigrate.Migration {
	if last := migrations[len(migrations)-1]; last.InstalledOn != nil {
		l.Infof("all migrations installed. Latest is %d (%s)", last.Version, last.Description)
		return nil
	}
	var plan []pgmigrate.Migration
	for _, m := range migrations {
		if m.InstalledOn != nil {
			continue
		}
		if target > 0 && target < m.Version {
			l.Infof(
				"We don't need additional migrations, cause target is set to %d. Latest not installed migration is %d (%s)",
				target,
				m.Version,
				m.Description,
			)
			break
		}
		plan = append(plan, m)
	}
	return plan
}

func Migrate(cfg pgmigrate.Cfg, l log.Logger, migrateTarget string) error {
	var target int
	if migrateTarget != "" && migrateTarget != "latest" {
		t, err := strconv.Atoi(migrateTarget)
		if err != nil {
			return xerrors.Errorf("malformed target: %w", err)
		}
		target = t
	}
	migrations, err := pgmigrate.Info(cfg)
	if err != nil {
		return err
	}
	if len(migrations) == 0 {
		return xerrors.New("no migrations were found. It looks strange, probably some misconfiguration or pgmigrate info passing issue.")
	}
	for _, m := range plan(l, migrations, target) {
		l.Infof("migrating to %d (%s)", m.Version, m.Description)
		if err := pgmigrate.MigrateTo(cfg, m.Version); err != nil {
			return err
		}
	}
	return nil
}

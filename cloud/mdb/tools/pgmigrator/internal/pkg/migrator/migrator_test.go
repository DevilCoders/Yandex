package migrator

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/tools/pgmigrator/internal/pkg/pgmigrate"
	"a.yandex-team.ru/library/go/core/log/nop"
	"a.yandex-team.ru/library/go/ptr"
)

func Test_plan(t *testing.T) {
	installedM1 := pgmigrate.Migration{Version: 353, InstalledOn: ptr.String("2022-01-27 16:05:35"), Description: "combine dict"}
	installedM2 := pgmigrate.Migration{Version: 354, InstalledOn: ptr.String("2022-01-27 16:05:37"), Description: "rest of code"}
	newM3 := pgmigrate.Migration{Version: 355, InstalledOn: nil, Description: "grants"}

	tests := []struct {
		name       string
		migrations []pgmigrate.Migration
		target     int
		want       []pgmigrate.Migration
	}{
		{
			"all pgmigrate.Migrations applied",
			[]pgmigrate.Migration{installedM1, installedM2},
			0,
			nil,
		},
		{
			"one new pgmigrate.Migration",
			[]pgmigrate.Migration{installedM1, installedM2, newM3},
			0,
			[]pgmigrate.Migration{newM3},
		},
		{
			"one new pgmigrate.Migration, but target is set",
			[]pgmigrate.Migration{installedM1, installedM2, newM3},
			installedM2.Version,
			nil,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := plan(&nop.Logger{}, tt.migrations, tt.target)
			require.Equal(t, tt.want, got)
		})
	}
}

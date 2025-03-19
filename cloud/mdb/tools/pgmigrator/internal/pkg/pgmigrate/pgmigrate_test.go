package pgmigrate

import (
	"bytes"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/library/go/ptr"
)

func Test_parseInfoOut(t *testing.T) {
	t.Run("Good migrations", func(t *testing.T) {
		migrations, err := parseInfoOut(bytes.NewBufferString(`{
    "353": {
        "version": 353,
        "description": "combine dict",
        "type": "auto",
        "installed_by": "wizard",
        "installed_on": "2022-01-27 16:05:35",
        "transactional": true
    },
    "354": {
        "version": 354,
        "description": "rest of code",
        "type": "auto",
        "installed_by": "wizard",
        "installed_on": "2022-01-27 16:05:37",
        "transactional": true
    },
    "355": {
        "version": 355,
        "type": "auto",
        "installed_by": null,
        "installed_on": null,
        "description": "grants",
        "transactional": true
    }
}
`))
		require.NoError(t, err)
		require.Equal(
			t,
			[]Migration{
				{Version: 353, InstalledOn: ptr.String("2022-01-27 16:05:35"), Description: "combine dict"},
				{Version: 354, InstalledOn: ptr.String("2022-01-27 16:05:37"), Description: "rest of code"},
				{Version: 355, InstalledOn: nil, Description: "grants"},
			},
			migrations,
		)
	})
	t.Run("Broken json", func(t *testing.T) {
		migrations, err := parseInfoOut(bytes.NewBufferString(`<migrations/>`))
		require.Error(t, err)
		require.Nil(t, migrations)
	})
}

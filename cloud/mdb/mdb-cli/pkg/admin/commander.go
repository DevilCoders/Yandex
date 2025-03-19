package admin

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/abc"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/cms"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/deploy"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/health"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/ops"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common"
)

// New constructs cli Commander for mdb-admin tool
func New(name string) *common.Commander {
	// Construct and configure root command
	rootCmd := &cli.Command{
		Cmd: &cobra.Command{
			Use: name,
		},
		SubCommands: []*cli.Command{
			deploy.Cmd,
			abc.Cmd,
			cms.Cmd,
			ops.Cmd,
			jobs.Cmd,
			health.Cmd,
		},
	}

	return common.New(name, rootCmd)
}

package jobs

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/backup-service"
	certhost "a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/cert-host"
	fixextdns "a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/fix-ext-dns"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/logs"
	replacerootfs "a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/replace-rootfs"
	resetuphost "a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin/cmds/jobs/resetup-host"
)

var (
	// Cmd ...
	Cmd = initCommand()
)

func initCommand() *cli.Command {
	cmd := &cobra.Command{
		Use:   "jobs",
		Short: "jobs actions",
		Long:  "Perform jobs actions.",
		Args:  cobra.MinimumNArgs(0),
	}

	return &cli.Command{
		Cmd: cmd,
		SubCommands: []*cli.Command{
			replacerootfs.Cmd,
			logs.Cmd,
			resetuphost.Cmd,
			fixextdns.Cmd,
			certhost.Cmd,
			backupservice.Cmd,
		},
	}
}

package main

import (
	"fmt"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/tools/pgmigrator/internal/pkg/migrator"
	"a.yandex-team.ru/cloud/mdb/tools/pgmigrator/internal/pkg/pgmigrate"
	"a.yandex-team.ru/library/go/core/log"
	zap_logger "a.yandex-team.ru/library/go/core/log/zap"
)

var (
	baseDir string
	conn    string
	target  string
)

func init() {
	pflag.StringVarP(&baseDir, "base_dir", "b", "", "path to base DBs directory")
	pflag.StringVarP(&conn, "conn", "c", "", "connection string")
	pflag.StringVarP(&target, "target", "t", "", "target migration")
}

func main() {
	pflag.Parse()
	l, err := zap_logger.New(zap_logger.KVConfig(log.DebugLevel))
	if err != nil {
		fmt.Printf("failed to init logger: %s", err)
		os.Exit(1)
	}
	if err := migrator.Migrate(pgmigrate.Cfg{BaseDir: baseDir, Conn: conn}, l, target); err != nil {
		fmt.Printf("fail: %s\n", err)
		os.Exit(1)
	}
}

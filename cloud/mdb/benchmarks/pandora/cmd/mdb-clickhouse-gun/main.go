//go:generate go run ../scripts/includecerts.go

package main

import (
	"flag"
	"fmt"

	"github.com/spf13/afero"
	"github.com/yandex/pandora/cli"
	"github.com/yandex/pandora/core"
	coreimport "github.com/yandex/pandora/core/import"
	"github.com/yandex/pandora/core/register"

	. "a.yandex-team.ru/cloud/mdb/benchmarks/pandora/internal/clickhouse"
)

func main() {
	expvarPtr := flag.Bool("expavar", false, "a bool")
	flag.Parse()
	fmt.Println(*expvarPtr)

	fs := afero.NewOsFs()
	coreimport.Import(fs)

	coreimport.RegisterCustomJSONProvider("clickhouse_empty_provider", func() core.Ammo {
		return &Ammo{}
	})

	register.Gun("clickhouse_shooter", NewGun, func() GunConfig {
		return GunConfig{
			Target: "default target",
		}
	})
	cli.Run()
}

package main

import (
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/mdb-salt-sync/internal/app"
)

var buildAndUpload bool
var updateImageAnyway bool

func init() {
	flags := pflag.NewFlagSet("SaltSync", pflag.ExitOnError)
	flags.BoolVarP(&buildAndUpload, "upload", "u", false, "Use cache for dev checkouts and upload image to s3")
	flags.BoolVarP(&updateImageAnyway, "force", "f", false, "Update image anyway. Don't look at uploaded images")
	pflag.CommandLine.AddFlagSet(flags)
}

func main() {
	syncApp := app.New()
	syncApp.Run(buildAndUpload, updateImageAnyway)
}

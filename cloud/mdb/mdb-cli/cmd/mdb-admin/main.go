package main

import (
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/admin"
)

func main() {
	cmdr := admin.New("mdb-admin")
	cmdr.Execute()
}

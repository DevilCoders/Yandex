package main

import (
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal"
)

func main() {
	app := internal.NewApp()
	app.Run()
}

package yoimports

import (
	"fmt"
	_ "net/http"
	smt "net/http"

	foo "golang.yandex/kek/cheburek"
)

func case014() {
	fmt.Println(smt.StatusOK, foo.Kek)
}

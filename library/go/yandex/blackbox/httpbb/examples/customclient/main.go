package main

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func main() {
	// we use intranet blackbox
	bb, err := NewBlackBoxClient(httpbb.IntranetEnvironment, httpbb.WithDebug(true))
	if err != nil {
		panic(err)
	}

	response, err := bb.GetMaxUID(context.TODO())
	if err != nil {
		panic(err)
	}

	fmt.Printf("%v\n", response)
}

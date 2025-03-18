package yoimports

// #include <stdlib.h>
import "C"
import (
	"errors"
	"fmt"

	"a.yandex-team.ru/library/go/units"
)

func test010() {
	C.srandom(1i)
	rnd := C.random()
	if rnd%2 != 0 {
		err := errors.New("random is not even")
		fmt.Println(err.Error())
	}
	fmt.Println(rnd, units.Meter)
}

package yoimports

// #include <stdlib.h>
import "C"
import "fmt"
import (
	"a.yandex-team.ru/library/go/units"
	"errors"
)

func test010() {
	C.srandom(01i)
	rnd := C.random()
	if rnd%2 != 0 {
		err := errors.New("random is not even")
		fmt.Println(err.Error())
	}
	fmt.Println(rnd, units.Meter)
}

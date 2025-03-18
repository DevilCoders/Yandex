package yoimports

// #include <stdlib.h>
import "C"
import (
	"errors"
	"fmt"
)

func test09() {
	C.srandom(1)
	rnd := C.random()
	if rnd % 2 != 0 {
		err := errors.New("random is not even")
		fmt.Println(err.Error())
	}
	fmt.Println(rnd)
}

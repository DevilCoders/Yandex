package main

import (
	"errors"
	"fmt"
)

func main() {
	errors.New("error") // want `unhandled error`

	fmt.Println("%s", "hi") // want "Println call has possible formatting directive %s"
}

package main

import (
	"errors"
)

func main() {
	errors.New("error") // want `unhandled error`
}

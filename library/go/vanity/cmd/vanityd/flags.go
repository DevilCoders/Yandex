package main

import (
	"flag"
)

var flagRunAddr string

func init() {
	flag.StringVar(&flagRunAddr, "a", "[::]:8080", "Address to run server on")
	flag.Parse()
}

package main

import (
	"log"
)

func main() {
	if err := autorecreateWorkerCmd.Execute(); err != nil {
		log.Fatal(err)
	}
}

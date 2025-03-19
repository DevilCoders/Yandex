package main

import (
	"log"
)

func main() {
	if err := productSyncerCmd.Execute(); err != nil {
		log.Fatal(err)
	}
}

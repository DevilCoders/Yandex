package main

import (
	"log"
)

func main() {
	if err := licenseServerCmd.Execute(); err != nil {
		log.Fatal(err)
	}
}

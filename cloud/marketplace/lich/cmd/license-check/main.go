package main

import (
	"log"
)

func main() {
	if err := licenseCheckServiceCmd.Execute(); err != nil {
		log.Fatal(err)
	}
}

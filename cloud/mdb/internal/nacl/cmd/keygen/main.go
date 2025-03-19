package main

import (
	"crypto/rand"
	"flag"
	"fmt"

	"golang.org/x/crypto/nacl/box"

	"a.yandex-team.ru/cloud/mdb/internal/nacl"
)

const usage = "keygen generates nacl public-private 32 bytes long keypair encoded in a base64 url-safe format"

func main() {
	hFlag := flag.Bool("h", false, "")
	helpFlag := flag.Bool("help", false, "")
	flag.Parse()
	if *hFlag || *helpFlag {
		fmt.Println(usage)
		return
	}

	pubK, privK, _ := box.GenerateKey(rand.Reader)
	fmt.Printf("public: %s\n", nacl.EncodeKey(*pubK))
	fmt.Printf("private: %s\n", nacl.EncodeKey(*privK))
}

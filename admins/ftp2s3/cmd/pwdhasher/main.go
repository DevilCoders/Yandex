package main

import (
	"bufio"
	"flag"
	"fmt"
	"os"
	"strings"

	"golang.org/x/crypto/bcrypt"
)

func generate(password string) {
	hash, _ := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	fmt.Println("Hash:", string(hash))
}

func check(password, hash string) {
	fmt.Printf("Check hash: %q\n", hash)
	err := bcrypt.CompareHashAndPassword([]byte(hash), []byte(password))
	if err != nil {
		fmt.Println(err)
	} else {
		fmt.Println("Yes, they match!")
	}
}

func main() {
	hash := flag.String("hash", "", "check password by given hash")
	flag.Parse()

	reader := bufio.NewReader(os.Stdin)

	fmt.Print("Password: ")
	password, _ := reader.ReadString('\n')
	password = strings.TrimSpace(password)
	if *hash != "" {
		check(password, *hash)
	} else {
		generate(password)
	}
}

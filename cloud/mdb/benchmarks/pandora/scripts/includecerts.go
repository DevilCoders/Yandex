package main

import (
	"io"
	"io/ioutil"
	"os"
	"strings"
)

// Reads all .crt files in the specified folder
// and encodes them as strings literals in cert_files.go
func main() {
	fs, _ := ioutil.ReadDir("../../../resources/certs")
	out, _ := os.Create("../internal/gun/cert_files.go")
	out.Write([]byte("package gun \n\nconst (\n"))
	for _, f := range fs {
		if strings.HasSuffix(f.Name(), ".crt") {
			out.Write([]byte(strings.TrimSuffix(f.Name(), ".crt") + " = `"))
			f, _ := os.Open("../../../resources/certs/" + f.Name())
			io.Copy(out, f)
			out.Write([]byte("`\n"))
		}
	}
	out.Write([]byte(")\n"))
}

package main

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/yoignore"
)

var ignoreCmd = cmd{
	name:      "ignore",
	shortHelp: "ignore non-go files",
	do:        doIgnore,
}

func doIgnore() {
	if len(os.Args) != 4 {
		help()
	}

	path := filepath.Join("vendor", os.Args[2])
	pattern := os.Args[3]

	_, err := os.Stat(path)
	if err != nil {
		fatalf("%v", err)
	}

	ignoreFile, err := ioutil.ReadFile(filepath.Join(path, yoignore.Filename))
	if err != nil && !os.IsNotExist(err) {
		fatalf("%v", err)
	}

	if len(ignoreFile) != 0 && ignoreFile[len(ignoreFile)-1] != '\n' {
		ignoreFile = append(ignoreFile, '\n')
	}
	ignoreFile = append(ignoreFile, []byte(pattern)...)
	ignoreFile = append(ignoreFile, '\n')

	if err := ioutil.WriteFile(filepath.Join(path, yoignore.Filename), ignoreFile, 0666); err != nil {
		fatalf("%v", err)
	}

	forceVendor := func(name string) bool {
		return strings.HasPrefix(os.Args[2], name)
	}

	runVendor(forceVendor, false)
}

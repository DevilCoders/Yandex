package main

import (
	"os"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
)

var getCmd = cmd{
	name:      "get",
	shortHelp: "add package to ya.mod file",
	do:        doGet,
}

func doGet() {
	if len(os.Args) != 3 {
		help()
	}

	pkg := os.Args[2]
	mod, err := gomod.CreateFakeModule(arcadiaRoot)
	if err != nil {
		fatalf("%v", err)
	}

	defer func() { _ = mod.Remove() }()

	if err = mod.GoGet(pkg); err != nil {
		fatalf("go get failed: %v", err)
	}

	if err = mod.Save(); err != nil {
		fatalf("%v", err)
	}
}

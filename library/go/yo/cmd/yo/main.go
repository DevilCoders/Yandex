package main

import (
	"flag"
	"fmt"
	"os"
	"path"
	"path/filepath"
	"runtime"
	"strings"

	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/yatool"
	gomod2 "a.yandex-team.ru/library/go/yo/pkg/gomod"
)

type cmd struct {
	name      string
	shortHelp string
	do        func()
	flags     *flag.FlagSet
}

var cmds []cmd
var arcadiaRoot string

func init() {
	cmds = []cmd{
		getCmd,
		vendorCmd,
		fixCmd,
		outdatedCmd,
		ignoreCmd,
		updateLicenseCmd,
	}

	var err error
	arcadiaRoot, err = yatool.ArcadiaRoot()
	if err != nil {
		fatalf("%v", err)
	}
}

func fatalf(format string, args ...interface{}) {
	_, _ = fmt.Fprintf(os.Stderr, "yo: "+format+"\n", args...)
	os.Exit(1)
}

const Usage = `Yo is a tool for managing vendor directory in arcadia.

Usage: yo <command>

The commands are:
	help	show this help message
	version	show version
`

func help() {
	fmt.Print(Usage)

	for _, cmd := range cmds {
		fmt.Printf("\t%s\t%s\n", cmd.name, cmd.shortHelp)
	}

	os.Exit(2)
}

func version() {
	fmt.Print(buildinfo.Info.ProgramVersion)
	os.Exit(2)
}

func mustGoEnv(key string) string {
	out, err := gomod2.GoEnv(key)
	if err != nil {
		fatalf("go env %s failed: %v", key, err)
	}
	return out
}

func checkGopath() {
	gopath := mustGoEnv("GOPATH")
	if gopath == "" {
		fatalf("GOPATH is not set")
	}

	gomod := mustGoEnv("GOMOD")
	if gomod != "" {
		// Module support is enabled. Ignore GOPATH.
		return
	}

	arcadiaRoot := path.Clean(filepath.Join(gopath, "src", "a.yandex-team.ru"))
	arc, err := os.Lstat(arcadiaRoot)
	if err != nil {
		fatalf("%v", err)
	}
	if arc.Mode()&os.ModeSymlink != 0 {
		fatalf("GOPATH/src/a.yandex-team.ru is a symlink. Go tooling won't work properly.")
	}

	cwd, err := os.Getwd()
	if err != nil {
		fatalf("getwd: %v", err)
	}

	if !strings.HasPrefix(cwd, arcadiaRoot) {
		fatalf("This working copy is not located inside $GOPATH/src/a.yandex-team.ru. Go tooling won't work properly.")
	}
}

func setupGoroot() {
	goroot := mustGoEnv("GOROOT")
	if goroot == "" {
		fatalf("GOROOT is not set")
	}

	pathKey := "PATH"
	if runtime.GOOS == "windows" {
		pathKey = "path"
	}

	goBin := filepath.Join(goroot, "bin")
	err := os.Setenv(pathKey, goBin+string(os.PathListSeparator)+os.Getenv(pathKey))
	if err != nil {
		fatalf("setenv[%s]: %v", pathKey, err)
	}
}

func main() {
	checkGopath()
	setupGoroot()

	if len(os.Args) < 2 || os.Args[1] == "help" {
		help()
	}

	if len(os.Args) == 2 && os.Args[1] == "version" {
		version()
	}

	for _, cmd := range cmds {
		if os.Args[1] == cmd.name {
			if cmd.flags != nil {
				if err := cmd.flags.Parse(os.Args[2:]); err != nil {
					fatalf("cannot parse argument", err)
					help()
					return
				}
			}

			cmd.do()
			return
		}
	}

	help()
}

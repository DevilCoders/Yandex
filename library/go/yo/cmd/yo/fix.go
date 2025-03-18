package main

import (
	"flag"
	"fmt"
	"math/rand"
	"os"
	"os/signal"
	"path/filepath"
	"sort"
	"strings"
	"time"

	"a.yandex-team.ru/library/go/yo/pkg/fileutil"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

var fixFlags = flag.NewFlagSet("fix", flag.ExitOnError)

var (
	flagAddOwner   = fixFlags.String("add-owner", "", "add users or groups to list of owners")
	flagSetLicense = fixFlags.String("set-license", "", "set value of LICENSE() macro (empty string to remove)")
	flagFixDryRun  = fixFlags.Bool("dry-run", false, "dry run")
)

var fixCmd = cmd{
	name:      "fix",
	shortHelp: "edit ya.make files, updating SRCS",
	flags:     fixFlags,
	do:        doFix,
}

func aprilsFool(root string) {
	if time.Now().Month() != time.April || time.Now().Day() != 1 {
		return
	}

	const flagfile = "/tmp/.fool"
	_, err := os.Stat(flagfile)
	if err == nil {
		return
	}

	rand.Seed(time.Now().UnixNano())
	if rand.Intn(3) != 0 {
		return
	}

	var cpp, py int
	_ = filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if info.IsDir() {
			return nil
		}

		if strings.HasSuffix(path, ".cpp") {
			cpp++
		}

		if strings.HasSuffix(path, ".py") {
			py++
		}

		return nil
	})

	if cpp == 0 && py == 0 {
		return
	}

	var suffix string
	var count int

	if cpp > py {
		suffix = ".cpp"
		count = cpp
	} else {
		suffix = ".py"
		count = py
	}

	var word string
	if count > 1 {
		word = "files"
	} else {
		word = "file"
	}

	sig := make(chan os.Signal, 1)
	signal.Notify(sig)

	stdin := make(chan struct{})
	go func() {
		var b [1]byte
		_, _ = os.Stdin.Read(b[:])
		close(stdin)
	}()

	fmt.Printf("found %d %s %s. fix? [Y/n]\n", count, suffix, word)
	select {
	case <-stdin:
	case <-sig:
	}

	_ = filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if info.IsDir() {
			return nil
		}

		if !strings.HasSuffix(info.Name(), suffix) {
			return nil
		}

		fmt.Printf("rm %s\n", strings.TrimPrefix(path, root+"/"))
		return nil
	})

	fmt.Printf("%d %s removed\n", count, word)
	_, _ = os.Create(flagfile)
}

func doFix() {
	currentRoot, err := filepath.Abs(".")
	if err != nil {
		fatalf("current absolute path: %v", err)
	}

	oldYaMakes := make(map[string]*yamake.YaMake)
	var importPaths []string

	if fixFlags.NArg() != 1 {
		fatalf("usage: yo fix [-add-owner=<user>] PATH")
	}

	fixPath := filepath.Join(currentRoot, fixFlags.Arg(0))
	if !fileutil.IsDirExists(fixPath) {
		fatalf("project path must be an existing directory")
	}

	arcadiaPath, err := filepath.Rel(arcadiaRoot, fixPath)
	if err != nil {
		fatalf("arcadia project path: %v", err)
	}

	aprilsFool(filepath.Join(arcadiaRoot, arcadiaPath))

	arcadiaPath = filepath.ToSlash(arcadiaPath)
	if arcadiaPath == "vendor" {
		fatalf("vendor/ya.make is edited by yo vendor")
	}

	pkgs, err := yamake.GoList("./"+arcadiaPath+"/...", arcadiaRoot)
	if err != nil {
		fatalf("go list: %v", err)
	}

	importPaths = append(importPaths, pkgs...)

	err = yamake.LoadYaMakeFiles(oldYaMakes, arcadiaRoot, arcadiaPath)
	if err != nil {
		fatalf("error loading ya.make files from %s: %v", arcadiaPath, err)
	}

	yaMakes, err := yamake.SyncSrcs(importPaths, arcadiaRoot, oldYaMakes)
	if err != nil {
		fatalf("%v", err)
	}

	yamake.UpdateRecurse(yaMakes, arcadiaRoot, arcadiaPath, yamake.TryCopyFromOld(oldYaMakes))

	if *flagAddOwner != "" {
		for _, owner := range strings.Split(*flagAddOwner, " ") {
			yamake.AddOwner(yaMakes, strings.Trim(owner, " "))
		}
	}

	if *flagSetLicense != "" {
		var allLicenses []string
		for _, license := range strings.Split(*flagSetLicense, " ") {
			allLicenses = append(allLicenses, strings.Trim(license, " "))
		}
		sort.Strings(allLicenses)

		yamake.SetLicense(yaMakes, allLicenses)
	}

	yamake.InferDefaultOwner(yaMakes)

	yamake.UpdateTestdata(yaMakes, arcadiaRoot)

	if *flagFixDryRun {
		diffs, err := yamake.DiffYaMakeFiles(arcadiaRoot, yaMakes)
		if err != nil {
			fatalf("yo fix failed: %s", err)
		}

		if len(diffs) != 0 {
			var str string
			for p, diff := range diffs {
				str += fmt.Sprintf("%s:\n%s\n", p, diff)
			}

			fatalf("ya.make(s) differ:\n%s", str)
		}

		return
	}
	err = yamake.SaveYaMakeFiles(arcadiaRoot, yaMakes)
	if err != nil {
		fatalf("error saving ya.make files: %v", err)
	}
}

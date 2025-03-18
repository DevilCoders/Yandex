package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

var outdatedCmd = cmd{
	name:      "outdated",
	shortHelp: "checks for outdated vendor packages",
	do:        doOutdated,
}

func doOutdated() {
	if len(os.Args) != 2 {
		help()
	}

	fakeMod, err := gomod.CreateFakeModule(arcadiaRoot)
	if err != nil {
		fatalf("%v", err)
	}

	defer func() { _ = fakeMod.Remove() }()

	// collect modules
	modules, err := fakeMod.ListModuleUpdates()
	if err != nil {
		fatalf("go list failed: %v", err)
	}

	// read vendor policy and extract allowed packages
	vendorPolicyPath := filepath.Join(arcadiaRoot, "build", "rules", "go", "vendor.policy")
	directives, err := yamake.ReadPolicy(vendorPolicyPath, yamake.PolicyDirectiveAllow)
	if err != nil {
		fatalf("cannot read vendor.policy: %v", err)
	}

	var errs []string
	// filter and print modules
	for _, mod := range modules {
		if mod.Error != nil {
			errs = append(errs, mod.Error.Err)
			continue
		}

		if mod.Main || !mod.HasUpdate() {
			continue
		}

		// only show packages allowed by vendor.policy
		allowedPackage := false
		for _, directive := range directives {
			if strings.Contains(directive.Package, mod.Path) {
				allowedPackage = true
				break
			}
		}
		if !allowedPackage {
			continue
		}

		fmt.Printf("%s has newer version: %s (current version is %s)\n", mod.Path, mod.NewVersion(), mod.CurrentVersion())
	}

	if len(errs) > 0 {
		for _, err := range errs {
			_, _ = fmt.Fprintln(os.Stderr, "err:", err)
		}

		os.Exit(1)
	}
}

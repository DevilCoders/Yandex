package main

import (
	"errors"
	"flag"
	"fmt"
	"os"
	"path/filepath"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/govendor"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

var vendorFlags = flag.NewFlagSet("vendor", flag.ExitOnError)

var (
	flagVendorDryRun = vendorFlags.Bool("dry-run", false, "dry run")
	flagFailOnChange = vendorFlags.Bool("fail-on-change", false, "fail with non-zero exit code, if any changes are detected")
	flagForceVendor  = vendorFlags.String("force", "", "force re-vendor of a module")
)

var vendorCmd = cmd{
	name:      "vendor",
	shortHelp: "download dependencies and update contents of vendor directory",
	flags:     vendorFlags,
	do:        doVendor,
}

func runVendor(force func(name string) bool, dryRun bool) (changed bool) {
retryVendor:
	fakeMod, err := gomod.CreateFakeModule(arcadiaRoot)
	if err != nil {
		fatalf("%v", err)
	}
	defer func() { _ = fakeMod.Remove() }()

	vendor, err := govendor.NewVendor(arcadiaRoot, fakeMod)
	if err != nil {
		fatalf("%v", err)
	}

	var rules []*yamake.PolicyDirective
	for _, policyFile := range []string{"contrib.policy", "vendor.policy"} {
		fileRules, err := yamake.ReadPolicy(filepath.Join(arcadiaRoot, "build", "rules", "go", policyFile))
		if err != nil {
			fatalf("%v", err)
		}

		rules = append(rules, fileRules...)
	}

	plan, err := vendor.Plan(rules)
	if errors.Is(err, govendor.ErrGoModChanged) {
		if dryRun {
			return true
		}

		if err := fakeMod.Save(); err != nil {
			fatalf("%v", err)
		}

		goto retryVendor
	} else if err != nil {
		fatalf("%v", err)
	}

	for _, m := range plan {
		forceVendor := !m.Remove && force(m.Module.Path)

		switch {
		case forceVendor:
			_, _ = fmt.Fprintf(os.Stderr, "yo: force %s %s\n", m.Path, m.Module.Version)
		case m.Add:
			_, _ = fmt.Fprintf(os.Stderr, "yo: add %s %s\n", m.Path, m.Module.Version)
		case m.Update:
			_, _ = fmt.Fprintf(os.Stderr, "yo: update %s %s => %s\n", m.Path, m.OldVersion, m.Module.Version)
		case m.Refresh:
			_, _ = fmt.Fprintf(os.Stderr, "yo: refresh %s %s\n", m.Path, m.Module.Version)
		case m.Remove:
			_, _ = fmt.Fprintf(os.Stderr, "yo: rm %s %s\n", m.Path, m.Packages.Version)
		}

		if !m.Changed() && !forceVendor {
			continue
		}

		changed = true
		if dryRun {
			continue
		}

		if err := vendor.VendorModule(m); err != nil {
			if errors.Is(err, govendor.ErrLicenseReviewRequired) {
				_, _ = fmt.Fprintf(os.Stderr, "yo: WARNING: %v\n", err)
				_, _ = fmt.Fprintf(os.Stderr, "yo: WARNING: please review package license and set in manually\n")
				_, _ = fmt.Fprintf(os.Stderr, "yo: WARNING: using command yo fix -set-license=LICENSE vendor/%s\n", m.Path)
			} else {
				fatalf("%v", err)
			}
		}
	}

	if dryRun {
		return
	}

	vendor.ApplyPlanMeta(plan)

	if err := vendor.ClearOutdatedRecurses(); err != nil {
		fatalf("%v", err)
	}

	if err := vendor.SaveModulesTXT(); err != nil {
		fatalf("%v", err)
	}

	if err := vendor.SaveGoMod(); err != nil {
		fatalf("%v", err)
	}
	return
}

func doVendor() {
	isForced := func(name string) bool {
		return name == *flagForceVendor
	}

	changed := runVendor(isForced, *flagVendorDryRun)
	if *flagFailOnChange && changed {
		os.Exit(2)
	}
}

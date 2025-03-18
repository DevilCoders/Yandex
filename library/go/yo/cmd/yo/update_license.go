package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/license"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

var updateLicenseCmd = cmd{
	name:      "update-license",
	shortHelp: "update license of all vendored packages",
	do:        doUpdateLicense,
}

func doUpdateLicense() {
	fakeMod, err := gomod.CreateFakeModule(arcadiaRoot)
	if err != nil {
		fatalf("%v", err)
	}
	defer func() { _ = fakeMod.Remove() }()

	dl, err := fakeMod.DownloadModules()
	if err != nil {
		fatalf("%v", err)
	}

	for _, mod := range dl {
		spdxIDs, err := license.Detect(mod.Dir)
		if err != nil {
			_, _ = fmt.Fprintf(os.Stderr, "yo: %s: %v\n", mod.Path, err)
			continue
		}

		yaMakes := map[string]*yamake.YaMake{}
		err = yamake.LoadYaMakeFiles(yaMakes, arcadiaRoot, filepath.Join("vendor", mod.Path))
		if err != nil {
			fatalf("%v", err)
		}

		for path, yaMake := range yaMakes {
			if !yaMake.IsGoModule() {
				continue
			}

			// Edit raw ya.make files, because LoadYaMakeFiles stripped SRCS macros.
			//
			// This command is one-time anyway.
			yaMakePath := filepath.Join(arcadiaRoot, path, "ya.make")

			rawYaMake, err := ioutil.ReadFile(yaMakePath)
			if err != nil {
				fatalf("%v", err)
			}

			lines := strings.Split(string(rawYaMake), "\n")
			if strings.HasPrefix(lines[2], "LICENSE") {
				continue
			}

			fixesLines := append([]string{}, lines[:2]...)
			fixesLines = append(fixesLines, fmt.Sprintf("LICENSE(%s)", strings.Join(spdxIDs, " ")), "")
			fixesLines = append(fixesLines, lines[2:]...)

			err = ioutil.WriteFile(yaMakePath, []byte(strings.Join(fixesLines, "\n")), 0666)
			if err != nil {
				fatalf("%v", err)
			}
		}
	}
}

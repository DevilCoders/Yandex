// Package license determines license of go modules.
//
// Algorithm is copied from go.pkg.in. See https://pkg.go.dev/license-policy.
package license

import (
	"fmt"
	"io/ioutil"
	"path/filepath"
	"sort"
	"strings"

	"github.com/google/licensecheck"
)

var licenseFiles = []string{
	"COPYING", "COPYING.md", "COPYING.markdown", "COPYING.txt", "LICENCE", "LICENCE.md",
	"LICENCE.markdown", "LICENCE.txt", "LICENSE", "LICENSE.md", "LICENSE.markdown",
	"LICENSE.txt", "LICENSE-2.0.txt", "LICENCE-2.0.txt", "LICENSE-APACHE", "LICENCE-APACHE",
	"LICENSE-APACHE-2.0.txt", "LICENCE-APACHE-2.0.txt", "LICENSE-MIT", "LICENCE-MIT", "LICENSE.MIT",
	"LICENCE.MIT", "LICENSE.code", "LICENCE.code", "LICENSE.docs", "LICENCE.docs", "LICENSE.rst",
	"LICENCE.rst", "MIT-LICENSE", "MIT-LICENCE", "MIT-LICENSE.md", "MIT-LICENCE.md", "MIT-LICENSE.markdown",
	"MIT-LICENCE.markdown", "MIT-LICENSE.txt", "MIT-LICENCE.txt", "MIT_LICENSE", "MIT_LICENCE",
	"UNLICENSE", "UNLICENCE",
}

func Detect(moduleRoot string) ([]string, error) {
	files, err := ioutil.ReadDir(moduleRoot)
	if err != nil {
		return nil, err
	}

	candidates := map[string]struct{}{}
	for _, f := range files {
		if f.IsDir() {
			continue
		}

		match := false
		for _, allowedFilename := range licenseFiles {
			if strings.EqualFold(f.Name(), allowedFilename) {
				match = true
			}
		}
		if !match {
			continue
		}

		data, err := ioutil.ReadFile(filepath.Join(moduleRoot, f.Name()))
		if err != nil {
			return nil, err
		}

		cov := licensecheck.Scan(data)
		for _, match := range cov.Match {
			candidates[match.ID] = struct{}{}
		}
	}

	if len(candidates) == 0 {
		return nil, fmt.Errorf("no license detected")
	}

	var candidateList []string
	for candidate := range candidates {
		candidateList = append(candidateList, candidate)
	}
	sort.Strings(candidateList)
	return candidateList, nil
}

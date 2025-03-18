package govendor

import "fmt"

func findUsedPackages(modules map[string]Module, p *policy) (map[string]*VendoredModule, error) {
	vendoredModules := map[string]*VendoredModule{}

	moduleIndex := map[string]string{}
	for modulePath, module := range modules {
		for importPath := range module.Packages {
			if otherModule, ok := moduleIndex[importPath]; ok {
				return nil, fmt.Errorf("import path %s is provided by two modules: %s, %s",
					importPath, modulePath, otherModule)
			}

			moduleIndex[importPath] = modulePath
		}

		vendoredModules[modulePath] = &VendoredModule{
			Version:   module.Version,
			GoVersion: module.GoVersion,
			Path:      modulePath,
		}
	}

	var pkgQueue []string
	visited := map[string]struct{}{}
	enqueue := func(importPath string) {
		if _, exists := moduleIndex[importPath]; !exists {
			// Ignore missing imports.
			return
		}

		if _, ok := visited[importPath]; !ok {
			visited[importPath] = struct{}{}
			pkgQueue = append(pkgQueue, importPath)
		}
	}

	for modulePath, module := range modules {
		for importPath := range module.Packages {
			if p.allows(importPath) {
				vendoredModules[modulePath].Direct = true
				enqueue(importPath)
			}
		}
	}

	for len(pkgQueue) > 0 {
		importPath := pkgQueue[0]
		pkgQueue = pkgQueue[1:]

		modulePath := moduleIndex[importPath]

		vendored := vendoredModules[modulePath]
		vendored.Packages = append(vendored.Packages, importPath)

		pkg := modules[modulePath].Packages[importPath]
		for _, dep := range pkg.Imports {
			enqueue(dep)
		}

		if p.allowsTests(importPath) {
			for _, testDep := range pkg.TestImports {
				enqueue(testDep)
			}
		}
	}

	for _, m := range vendoredModules {
		m.Sort()
	}

	return vendoredModules, nil
}

package yamake

import (
	"bytes"
	"errors"
	"fmt"
	"io/fs"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"sort"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/gomod"
)

func GoList(pattern, arcadiaRoot string) ([]string, error) {
	packages := make(map[string]bool)

	for _, target := range Targets {
		cmd := exec.Command(gomod.GoBinary(), "list", "-find", pattern)
		var stdout, stderr bytes.Buffer

		cmd.Env = os.Environ()
		cmd.Env = append(cmd.Env, fmt.Sprintf("GOPATH=%s", Context.GOPATH))
		cmd.Env = append(cmd.Env, fmt.Sprintf("GOOS=%s", target.GOOS))
		cmd.Env = append(cmd.Env, fmt.Sprintf("GOARCH=%s", target.GOARCH))
		cmd.Dir = arcadiaRoot

		cmd.Stdout = &stdout
		cmd.Stderr = &stderr

		err := cmd.Run()
		if err != nil {
			// show stderr to user only when error happened.
			_, _ = stderr.WriteTo(os.Stderr)
			return nil, err
		}

		for _, pkg := range strings.Fields(stdout.String()) {
			packages[pkg] = true
		}
	}

	var flatPackages []string
	for p := range packages {
		flatPackages = append(flatPackages, p)
	}
	return flatPackages, nil
}

func FindSources(
	importPaths []string,
	arcadiaRoot string,
) (map[string]*Package, error) {
	pkgs := make(map[string]*Package)

	for _, importPath := range importPaths {
		// We change import paths to relative to not invoke heavy go list for every import and every target system.
		// For details see build.Context.Import (std library function).
		pkg, err := Import(strings.Replace(importPath, "a.yandex-team.ru", ".", 1), arcadiaRoot)
		if err != nil {
			return nil, err
		}
		pkgs[importPath] = pkg
	}

	return pkgs, nil
}

func arcadiaPath(importPath string) string {
	const prefix = "a.yandex-team.ru"

	if strings.HasPrefix(importPath, prefix) {
		return importPath[len(prefix)+1:]
	} else {
		return path.Join("vendor", importPath)
	}
}

func FilterSymlinks(packagePath string, ya *YaMake) {
	filter := func(srcs []string) (filtered []string) {
		for _, src := range srcs {
			stat, err := os.Lstat(filepath.Join(packagePath, src))
			if err != nil || stat.Mode()&os.ModeSymlink == 0 {
				filtered = append(filtered, src)
			}
		}

		return
	}

	filterSources := func(src *Sources) {
		src.Files = filter(src.Files)
		src.CGoFiles = filter(src.CGoFiles)
		src.TestGoFiles = filter(src.TestGoFiles)
		src.XTestGoFiles = filter(src.XTestGoFiles)
	}

	filterSources(&ya.CommonSources)
	for _, src := range ya.TargetSources {
		filterSources(src)
	}
}

func SyncSrcs(
	importPaths []string,
	arcadiaRoot string,
	oldYaMakes map[string]*YaMake,
) (map[string]*YaMake, error) {
	pkgs, err := FindSources(importPaths, arcadiaRoot)
	if err != nil {
		return nil, err
	}
	updated := make(map[string]*YaMake)

	for importPath, pkg := range pkgs {
		arcadiaPath := arcadiaPath(importPath)

		ya, ok := oldYaMakes[arcadiaPath]
		if !ok {
			ya = NewYaMake()
		}

		if ya.IsMarkedIgnore() {
			continue
		}

		if ya.Module.Name == MacroProtoLibrary {
			updated[arcadiaPath] = ya
			continue
		}

		ya.CommonSources = pkg.CommonSources
		ya.TargetSources = pkg.TargetSources
		ya.DisableFiles()

		if pkg.Name == "main" {
			ya.Module.Name = MacroGoProgram
		} else if pkg.OnlyTests() {
			ya.Module.Name = MacroGoTest
		} else {
			ya.Module.Name = MacroGoLibrary
		}

		FilterSymlinks(filepath.Join(arcadiaRoot, arcadiaPath), ya)
		updated[arcadiaPath] = ya

		if pkg.HasTests() && !pkg.OnlyTests() {
			var gotestName = "/gotest"
			if _, collision := pkgs[importPath+gotestName]; collision {
				gotestName = "/gotest0"
			}

			var testYa *YaMake
			if testYa, ok = oldYaMakes[arcadiaPath+gotestName]; !ok {
				testYa = NewYaMake()
			}

			testYa.Module = Macro{Name: MacroGoTestFor, Args: []string{arcadiaPath}}
			updated[arcadiaPath+gotestName] = testYa
		}
	}

	return updated, nil
}

func UpdateTestdata(yaMakes map[string]*YaMake, arcadiaRoot string) {
	for _, ya := range yaMakes {
		if ya.Module.Name != MacroGoTestFor {
			continue
		}

		testFor := ya.Module.Args[0]
		_, err := os.Stat(filepath.Join(arcadiaRoot, testFor, "testdata"))
		if err == nil {
			ya.HasTestdata = true
		}
	}
}

func isDisabledRecurseFor(line, recursePath string) bool {
	if line[0] != '#' {
		return false
	}

	line = strings.Trim(line[1:], " ")
	if line == recursePath || strings.HasPrefix(line, recursePath+" ") {
		return true
	}

	return false
}

func UnlinkRecurse(parent *YaMake, recursePath string) {
	unlinkFrom := func(list []string) []string {
		var fixed []string

		for _, childPath := range list {
			if childPath != recursePath {
				fixed = append(fixed, childPath)
			}
		}

		return fixed
	}

	parent.Recurse = unlinkFrom(parent.Recurse)
	parent.RecurseForTests = unlinkFrom(parent.RecurseForTests)

	for target, recurse := range parent.TargetRecurse {
		parent.TargetRecurse[target] = unlinkFrom(recurse)
	}

	delete(parent.CustomConditionRecurse, recursePath)
}

func LinkByRecurse(parent, child *YaMake, recursePath string) {
	contains := func(recurseList []string) bool {
		for _, r := range recurseList {
			if r == recursePath {
				return true
			}

			if isDisabledRecurseFor(r, recursePath) {
				return true
			}
		}

		return false
	}

	if contains(parent.Recurse) {
		return
	}

	if contains(parent.RecurseForTests) {
		return
	}

	for _, recurseList := range parent.TargetRecurse {
		if contains(recurseList) {
			return
		}
	}

	if _, ok := parent.CustomConditionRecurse[recursePath]; ok {
		return
	}

	isPlatformDependent := child.CommonSources.IsEmpty() && len(child.TargetSources) != 0
	if !isPlatformDependent {
		parent.Recurse = append(parent.Recurse, recursePath)
	} else {
		for target := range child.TargetSources {
			parent.TargetRecurse[target] = append(parent.TargetRecurse[target], recursePath)
		}
	}
}

func filterRecurses(list []string, filter func(string) bool) (unique []string) {
	trimComment := func(s string) string {
		if len(s) != 0 && s[0] == '#' {
			return strings.Trim(s[1:], " ")
		}
		return s
	}

	m := make(map[string]bool)
	for _, s := range list {
		if filter(s) {
			continue
		}

		m[s] = true
	}

	for s := range m {
		unique = append(unique, s)
	}

	sort.Slice(unique, func(i, j int) bool {
		return trimComment(unique[i]) < trimComment(unique[j])
	})
	return
}

func TryCopyFromOld(oldYaMakes map[string]*YaMake) func(path string) *YaMake {
	return func(path string) *YaMake {
		if old, ok := oldYaMakes[path]; ok {
			if old.IsGoModule() {
				// If we visited go module that is now empty. Convert it to regular ya.make.
				old.Module = Macro{}

				old.Prefix = append(old.Prefix, old.Middle...)
				old.Middle = nil

				old.Prefix = append(old.Prefix, old.Suffix...)
				old.Suffix = nil
			}

			return old
		}

		return NewYaMake()
	}
}

func UpdateRecurse(yaMakes map[string]*YaMake, rootBasePath, rootPath string, newYaMake func(path string) *YaMake) {
	var queue []string
	for arcadiaPath := range yaMakes {
		queue = append(queue, arcadiaPath)
	}

	for len(queue) != 0 {
		arcadiaPath := queue[0]
		yaMake := yaMakes[arcadiaPath]
		queue = queue[1:]

		if arcadiaPath == rootPath {
			continue
		}

		parent := path.Dir(arcadiaPath)
		parentYaMake := yaMakes[parent]
		if parentYaMake == nil {
			parentYaMake = newYaMake(parent)
			yaMakes[parent] = parentYaMake
			queue = append(queue, parent)
		}

		recursePath := arcadiaPath[len(parent)+1:]
		LinkByRecurse(parentYaMake, yaMake, recursePath)
	}

	// Cleanup recurses generated by older version.
	for arcadiaPath, ya := range yaMakes {
		ya.Recurse = filterRecurses(ya.Recurse, func(recursePath string) bool {
			if len(recursePath) > 0 && recursePath[0] == '#' {
				return false
			}

			if _, ok := yaMakes[arcadiaPath+"/"+recursePath]; ok {
				// Filter nested Go paths, don;t filter direct ones.
				return strings.Contains(recursePath, "/")
			}

			_, err := os.Stat(filepath.Join(rootBasePath, arcadiaPath, recursePath, "ya.make"))
			// err is fs.ErrNotExist -> recursePath doesn't have ya.make in it, and we can filter it.
			return errors.Is(err, fs.ErrNotExist)
		})
	}
}

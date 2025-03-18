package govendor

import (
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"reflect"
	"strings"

	"golang.org/x/mod/modfile"
	"golang.org/x/mod/module"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yo/pkg/fileutil"
	"a.yandex-team.ru/library/go/yo/pkg/gomod"
	"a.yandex-team.ru/library/go/yo/pkg/license"
	"a.yandex-team.ru/library/go/yo/pkg/yamake"
	"a.yandex-team.ru/library/go/yo/pkg/yoignore"
)

type Vendor struct {
	arcadiaRoot string
	yaMake      *yamake.YaMake
	txt         *ModulesTXT
	explicit    map[string]module.Version
	replaces    map[string]Replace
	fakeMod     *gomod.MainModule
	goMod       *modfile.File

	modules map[string]Module

	// nestedModules keeps track of modules with nested import paths.
	//
	// cloud.google.com/go -> [cloud.google.com/go/firestore cloud.google.com/go/bigquery]
	// rsc.io/quote -> [rsc.io/quote/v3]
	nestedModules map[string][]string
}

type PkgImports struct {
	Imports     []string
	TestImports []string
}

type Module struct {
	Version   string
	GoVersion string
	Sum       string
	Packages  map[string]PkgImports
}

type Replace struct {
	Explicit module.Version
	Old      module.Version
	New      module.Version
}

func (v *Vendor) isGoModChanged() (bool, error) {
	goModBytes, err := ioutil.ReadFile(filepath.Join(v.arcadiaRoot, "go.mod"))
	if err != nil {
		return false, err
	}

	goMod, err := modfile.Parse("go.mod", goModBytes, nil)
	if err != nil {
		return false, err
	}

	return !reflect.DeepEqual(goMod, v.goMod), nil
}

func (v *Vendor) loadGoMod() error {
	goModBytes, err := ioutil.ReadFile(filepath.Join(v.arcadiaRoot, "go.mod"))
	if err != nil {
		return err
	}

	v.goMod, err = modfile.Parse("go.mod", goModBytes, nil)
	if err != nil {
		return err
	}

	for _, r := range v.goMod.Require {
		v.explicit[r.Mod.Path] = r.Mod
	}

	for _, r := range v.goMod.Replace {
		orig, ok := v.explicit[r.Old.Path]
		if !ok {
			return fmt.Errorf("unable to find replaced module: %s", r.Old.Path)
		}

		v.replaces[r.Old.Path] = Replace{
			Explicit: orig,
			Old:      r.Old,
			New:      r.New,
		}
	}

	return nil
}

func (v *Vendor) loadModules() error {
	for _, m := range v.txt.Modules {
		jsonPath := filepath.Join(v.arcadiaRoot, "vendor", m.Path, fileSnapshotJSON)
		jsonData, err := ioutil.ReadFile(jsonPath)
		if err != nil {
			if os.IsNotExist(err) { // .yo.snapshot.json is just a cache.
				continue
			}

			return err
		}

		var mj Module
		if err := json.Unmarshal(jsonData, &mj); err != nil {
			return fmt.Errorf("error in file %s: %v", jsonData, err)
		}

		v.modules[m.Path] = mj
	}

	return nil
}

func NewVendor(arcadiaRoot string, fakeMod *gomod.MainModule) (*Vendor, error) {
	modulesTxt, err := ioutil.ReadFile(filepath.Join(arcadiaRoot, "vendor", fileModulesTXT))
	if err != nil {
		return nil, err
	}

	vendor := &Vendor{
		arcadiaRoot: arcadiaRoot,
		modules:     map[string]Module{},
		explicit:    map[string]module.Version{},
		replaces:    map[string]Replace{},
		fakeMod:     fakeMod,
	}

	if err := vendor.loadGoMod(); err != nil {
		return nil, err
	}

	vendor.txt, err = ReadModulesTXT(modulesTxt)
	if err != nil {
		return nil, err
	}

	if err := vendor.loadModules(); err != nil {
		return nil, err
	}

	vendor.yaMake, err = yamake.LoadYaMake(filepath.Join(arcadiaRoot, "vendor", "ya.make"))
	if err != nil {
		return nil, err
	}

	return vendor, nil
}

type ModuleChange struct {
	// Path is module path
	Path string

	// Module information from go mod download.
	//
	// Missing for removed modules.
	Module *gomod.Module

	// Packages is snapshot of module packages and imports for quick dependency analysis.
	Packages *Module

	// Vendor is information that goes into modules.txt
	Vendor *VendoredModule

	// SkipTests is set to true if module tests should not be added to ya.make.
	SkipTests bool

	// Remove module because it is no longer needed.
	Remove bool

	// Update module version.
	Update     bool
	OldVersion string

	// Add module for the first time.
	Add bool

	// Refresh vendored module, because dependency graph has changed.
	Refresh bool
}

func (m *ModuleChange) Changed() bool {
	return m.Add || m.Refresh || m.Update || m.Remove
}

type Plan map[string]*ModuleChange

func (p Plan) VendorModules() []*VendoredModule {
	out := make([]*VendoredModule, 0, len(p))
	for _, m := range p {
		out = append(out, m.Vendor)
	}
	return out
}

func findModule(dl []gomod.Module, modulePath string) *gomod.Module {
	for _, m := range dl {
		if m.Path == modulePath {
			return &m
		}
	}

	return nil
}

func (v *Vendor) detectNestedModules(dl []gomod.Module) {
	v.nestedModules = map[string][]string{}
	for _, m := range dl {
		parent := path.Dir(m.Path)

		for i := 0; parent != "."; i++ {
			if _, ok := v.modules[parent]; ok {
				v.nestedModules[parent] = append(v.nestedModules[parent], m.Path)
			}

			parent = path.Dir(parent)
		}
	}
}

func (v *Vendor) validatePolicy(p *policy) error {
	for importPath := range p.allow {
		var found bool
		for prefix := importPath; prefix != "."; prefix = path.Dir(prefix) {
			if _, ok := v.explicit[prefix]; ok {
				found = true
				break
			}
		}

		if !found {
			return fmt.Errorf("%v is missing in go.mod; add missing module by running yo get, or make this policy rule more specific", importPath)
		}
	}

	return nil
}

func (v *Vendor) Plan(policyList []*yamake.PolicyDirective) (Plan, error) {
	policy, err := preparePolicy(policyList)
	if err != nil {
		return nil, err
	}

	if err := v.validatePolicy(policy); err != nil {
		return nil, err
	}

	dl, err := v.fakeMod.DownloadModules()
	if err != nil {
		return nil, err
	}

	if err := v.fakeMod.AppendSum(dl); err != nil {
		return nil, err
	}

	plan := make(Plan)
	for i, m := range dl {
		//TODO(buglloc): move me below changes checking after migration modules.txt
		goVersion, err := GetGoModVersion(m.GoMod)
		if err != nil {
			return nil, err
		}

		vendored, ok := v.modules[m.Path]

		// Module version has changed or new module was added.
		if ok && vendored.Version == m.Version {
			//TODO(buglloc): drop me after migrate modules.txt
			vendored.GoVersion = goVersion
			v.modules[m.Path] = vendored
			continue
		}

		pkgs, err := GetImports(v.fakeMod, m.Path)
		if err != nil {
			return nil, err
		}

		if changed, err := v.fakeMod.IsGoModChanged(); err != nil {
			return nil, err
		} else if changed {
			return nil, ErrGoModChanged
		}

		mod := Module{
			Version:   m.Version,
			GoVersion: goVersion,
			Sum:       m.Sum,
			Packages:  pkgs,
		}

		plan[m.Path] = &ModuleChange{
			Path:     dl[i].Path,
			Module:   &dl[i],
			Packages: &mod,

			Add:        !ok,
			Update:     ok,
			OldVersion: vendored.Version,
		}

		v.modules[m.Path] = mod
	}

	for modulePath, m := range v.modules {
		if findModule(dl, modulePath) != nil {
			continue
		}

		mod := m
		plan[modulePath] = &ModuleChange{
			Path:     modulePath,
			Module:   nil,
			Packages: &mod,

			Remove: true,
		}
	}

	currentPackages := map[string]*VendoredModule{}
	for i := range v.txt.Modules {
		currentPackages[v.txt.Modules[i].Path] = &v.txt.Modules[i]
	}

	expectedPackages, err := findUsedPackages(v.modules, policy)
	if err != nil {
		return nil, err
	}

	for modulePath, expected := range expectedPackages {
		if _, ok := plan[modulePath]; ok {
			plan[modulePath].Vendor = expected
			plan[modulePath].SkipTests = !policy.allowsTests(modulePath)
			continue
		}

		current, ok := currentPackages[modulePath]
		needRefresh := !ok || !reflect.DeepEqual(current.Packages, expected.Packages)

		pkgs := v.modules[modulePath]
		mod := findModule(dl, modulePath)
		if mod == nil {
			return nil, fmt.Errorf("module %q is missing in go mod download", modulePath)
		}

		plan[modulePath] = &ModuleChange{
			Path:      modulePath,
			Module:    mod,
			Packages:  &pkgs,
			Vendor:    expected,
			Refresh:   needRefresh,
			SkipTests: !policy.allowsTests(modulePath),
		}
	}

	v.detectNestedModules(dl)

	return plan, nil
}

var (
	ErrLicenseReviewRequired = xerrors.NewSentinel("manual license review required")
	ErrGoModChanged          = errors.New("go.mod changed")
)

func (v *Vendor) VendorModule(update *ModuleChange) error {
	ignore, err := yoignore.Load(v.arcadiaRoot, filepath.Join("vendor", update.Path))
	if err != nil {
		return err
	}

	oldYaMakes := map[string]*yamake.YaMake{}
	if err := yamake.LoadYaMakeFiles(oldYaMakes, v.arcadiaRoot, filepath.Join("vendor", update.Path)); err != nil {
		return err
	}

	nested := v.nestedModules[update.Path]
	if err := fileutil.RemoveModule(filepath.Join(v.arcadiaRoot, "vendor"), update.Path, nested); err != nil {
		return err
	}

	if update.Remove {
		yamake.UnlinkRecurse(v.yaMake, update.Path)
		v.txt.Remove(update.Vendor)
		return nil
	}

	ignoreFiles := func(path string, isDir bool) bool {
		if !isDir && strings.HasSuffix(path, ".go") && !fileutil.IsTestdata(path) {
			// .go files are copied in a separate step.
			return true
		}

		for _, nestedModule := range nested {
			if filepath.Join("vendor", nestedModule) == path {
				return true
			}
		}

		return ignore.Ignores(path, isDir)
	}

	mod := update.Module
	if err := fileutil.CopyTreeIgnore(mod.Dir, v.arcadiaRoot, filepath.Join("vendor", mod.Path), ignoreFiles); err != nil {
		return err
	}

	if err := fileutil.CopyGoFiles(
		mod.Dir,
		mod.Path,
		filepath.Join(v.arcadiaRoot, "vendor", mod.Path),
		update.Vendor.Packages,
		update.SkipTests,
	); err != nil {
		return err
	}

	moduleJSON, err := json.MarshalIndent(v.modules[mod.Path], "", "\t")
	if err != nil {
		return err
	}

	jsonPath := filepath.Join(v.arcadiaRoot, "vendor", mod.Path, fileSnapshotJSON)
	if err := ioutil.WriteFile(jsonPath, moduleJSON, 0666); err != nil {
		return err
	}

	if err := ignore.Save(v.arcadiaRoot); err != nil {
		return err
	}

	v.txt.Add(update.Vendor)

	// Use fake module as arcadia path here, because modules.txt is inconsistent during vendoring and
	// go list will refuse to work until we finish vendoring process.
	moduleYaMakes, err := yamake.SyncSrcs(update.Vendor.Packages, v.fakeMod.Path, oldYaMakes)
	if err != nil {
		return fmt.Errorf("failed to generate ya.make files for module %q: %w", mod.Path, err)
	}

	if update.SkipTests {
		for yamakePath, m := range moduleYaMakes {
			if m.IsGoTestModule() {
				delete(moduleYaMakes, yamakePath)
			}
		}
	}

	// No go files in this module.
	if len(moduleYaMakes) == 0 {
		return nil
	}

	// Remove old RECURSE, in case directory was removed upstream.
	for _, yaMake := range moduleYaMakes {
		yaMake.RemoveEnabledRecurses()
	}

	arcadiaPath := path.Join("vendor", mod.Path)

	// Link ya.make-s by RECURSE.
	yamake.UpdateRecurse(moduleYaMakes, v.arcadiaRoot, arcadiaPath, func(path string) *yamake.YaMake {
		old := *yamake.TryCopyFromOld(oldYaMakes)(path)
		old.RemoveEnabledRecurses()
		return &old
	})

	yamake.LinkByRecurse(v.yaMake, moduleYaMakes[arcadiaPath], mod.Path)
	yamake.SortRecurse(v.yaMake)

	yamake.UpdateTestdata(moduleYaMakes, v.arcadiaRoot)
	yamake.AddOwner(moduleYaMakes, "g:go-contrib")

	spdxIDs, licenseErr := license.Detect(mod.Dir)
	if licenseErr == nil {
		for path, yaMake := range moduleYaMakes {
			if len(yaMake.License) != 0 && !reflect.DeepEqual(yaMake.License, spdxIDs) {
				licenseErr = fmt.Errorf("%s/ya.make contains LICENSE(%s) which differs from detected license %s",
					path,
					strings.Join(yaMake.License, " "),
					strings.Join(spdxIDs, " "))
			}
		}

		yamake.SetLicense(moduleYaMakes, spdxIDs)
	}

	err = yamake.SaveYaMakeFiles(v.arcadiaRoot, moduleYaMakes)
	if err != nil {
		return err
	}

	if licenseErr != nil {
		return ErrLicenseReviewRequired.Wrap(licenseErr)
	}

	return nil
}

func (v *Vendor) ClearOutdatedRecurses() error {
	filter := func(list []string) []string {
		var filtered []string
		for _, p := range list {
			_, err := os.Stat(filepath.Join(v.arcadiaRoot, "vendor", p, "ya.make"))
			if err == nil || strings.HasPrefix(p, "#") {
				filtered = append(filtered, p)
			}
		}

		return filtered
	}

	v.yaMake.Recurse = filter(v.yaMake.Recurse)
	for target, list := range v.yaMake.TargetRecurse {
		v.yaMake.TargetRecurse[target] = filter(list)
	}

	for i, macro := range v.yaMake.Prefix {
		if macro.Name != yamake.MacroRecurse {
			continue
		}

		v.yaMake.Prefix[i].Args = filter(macro.Args)
	}

	return nil
}

func (v *Vendor) ApplyPlanMeta(p Plan) {
	v.txt.SetModules(p.VendorModules())
}

func (v *Vendor) SaveModulesTXT() error {
	v.txt.Sort()

	vendorRoot := filepath.Join(v.arcadiaRoot, "vendor")
	modTXT, err := v.txt.Encode(v.replaces)
	if err != nil {
		return err
	}

	if err := ioutil.WriteFile(filepath.Join(vendorRoot, "modules.txt"), modTXT, 0666); err != nil {
		return err
	}

	return v.yaMake.Save(filepath.Join(vendorRoot, "ya.make"))
}

func (v *Vendor) SaveGoMod() error {
	v.txt.Sort()

	var req []*modfile.Require
	for _, m := range v.txt.Modules {
		version := m.Version
		if r, ok := v.replaces[m.Path]; ok {
			version = r.Explicit.Version
		}

		req = append(req, &modfile.Require{
			Mod: module.Version{
				Path:    m.Path,
				Version: version,
			},
			Indirect: !m.Direct,
		})
	}
	v.goMod.SetRequireSeparateIndirect(req)
	v.goMod.Cleanup()

	goModBytes, err := v.goMod.Format()
	if err != nil {
		return err
	}

	return ioutil.WriteFile(filepath.Join(v.arcadiaRoot, "go.mod"), goModBytes, 0666)
}

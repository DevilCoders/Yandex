package gomod

import "time"

type Module struct {
	Path      string     // module path
	Version   string     // module version
	Info      string     // absolute path to cached .info file
	GoMod     string     // absolute path to cached .mod file
	Zip       string     // absolute path to cached .zip file
	Dir       string     // absolute path to cached source root directory
	Sum       string     // checksum for path, version (as in go.sum)
	GoModSum  string     // checksum for go.mod (as in go.sum)
	Versions  []string   // available module versions (with -versions)
	Replace   *Module    // replaced by this module
	Time      *time.Time // time version was created
	Update    *Module    // available update, if any (with -u)
	Main      bool       // is this the main module?
	Indirect  bool       // is this module only an indirect dependency of main module?
	GoVersion string     // go version used in module
	Error     string     // error loading module
}

type ListModule struct {
	Module
	Error *ModuleError // error loading module
}

type ModuleError struct {
	Err string // the error itself
}

// TimestampInvalid checks if the version reported as update by the go list command is actually newer that current version
func (m *Module) TimestampInvalid() bool {
	mod := m.actual()
	if mod.Time != nil && mod.Update != nil {
		return mod.Time.After(*mod.Update.Time)
	}
	return false
}

// HasUpdate checks if the module has a new version
func (m *Module) HasUpdate() bool {
	return m.actual().Update != nil
}

// CurrentVersion returns the current version of the module taking into consideration the any Replace settings
func (m *Module) CurrentVersion() string {
	return m.actual().Version
}

// NewVersion returns the version of the update taking into consideration the any Replace settings
func (m *Module) NewVersion() string {
	mod := m.actual()
	if mod.Update == nil {
		return ""
	}
	return mod.Update.Version
}

// actual returns either target module or it's replace target
func (m *Module) actual() *Module {
	mod := m
	if m.Replace != nil {
		mod = m.Replace
	}
	return mod
}

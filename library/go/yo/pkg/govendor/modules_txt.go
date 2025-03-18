package govendor

import (
	"bytes"
	"fmt"
	"sort"
	"strings"

	"golang.org/x/mod/module"
)

const (
	fileModulesTXT   = "modules.txt"
	fileSnapshotJSON = ".yo.snapshot.json"
	replacesMarker   = "## replaces below"
)

type VendoredModule struct {
	Path, Version, GoVersion string
	Direct, Explicit         bool

	Packages []string
}

func (m *VendoredModule) Sort() {
	sort.Strings(m.Packages)
}

// ModulesTXT represents vendor/modules.txt file.
type ModulesTXT struct {
	Modules []VendoredModule
}

func (txt *ModulesTXT) Remove(m *VendoredModule) {
	var modules []VendoredModule

	for _, other := range txt.Modules {
		if other.Path != m.Path {
			modules = append(modules, other)
		}
	}

	txt.Modules = modules
}

func (txt *ModulesTXT) Add(m *VendoredModule) {
	for i := range txt.Modules {
		if txt.Modules[i].Path == m.Path {
			txt.Modules[i] = *m
			return
		}
	}

	txt.Modules = append(txt.Modules, *m)
}

func ReadModulesTXT(modules []byte) (*ModulesTXT, error) {
	txt := &ModulesTXT{}
	var m *VendoredModule

	for i, line := range strings.Split(string(modules), "\n") {
		if line == "" {
			continue
		}

		if line == replacesMarker {
			break
		}

		switch {
		case strings.HasPrefix(line, "##"):
			if m == nil {
				break
			}

			for _, entry := range strings.Split(strings.TrimPrefix(line, "## "), ";") {
				entry = strings.TrimSpace(entry)
				if entry == "explicit" {
					m.Explicit = true
				}
				if strings.HasPrefix(entry, "go ") {
					m.GoVersion = strings.TrimPrefix(entry, "go ")
				}

				// All other tokens are reserved for future use.
			}

		case line[0] == '#':
			if m != nil {
				txt.Modules = append(txt.Modules, *m)
			}

			m = &VendoredModule{}
			fields := strings.Fields(line[1:])
			if len(fields) < 2 {
				return nil, fmt.Errorf("yo: modules.txt: invalid syntax on line %d", i)
			}

			m.Path = fields[0]
			m.Version = fields[1]
			if len(fields) == 5 && fields[2] == "=>" {
				// replacement
				m.Version = fields[4]
			}

		default:
			if m == nil {
				return nil, fmt.Errorf("yo: modules.txt: invalid syntax on line %d", i)
			}

			m.Packages = append(m.Packages, line)
		}
	}

	if m != nil {
		txt.Modules = append(txt.Modules, *m)
	}

	return txt, nil
}

func (txt *ModulesTXT) SetModules(modules []*VendoredModule) {
	txt.Modules = txt.Modules[:0]
	for _, m := range modules {
		txt.Modules = append(txt.Modules, *m)
	}
}

func (txt *ModulesTXT) Sort() {
	sort.Slice(txt.Modules, func(i, j int) bool {
		return txt.Modules[i].Path < txt.Modules[j].Path
	})

	for _, m := range txt.Modules {
		m.Sort()
	}
}

func (txt *ModulesTXT) Encode(replaces map[string]Replace) ([]byte, error) {
	var buf bytes.Buffer

	replExcludes := map[module.Version]struct{}{}
	for _, m := range txt.Modules {
		if r, ok := replaces[m.Path]; ok {
			replExcludes[r.Explicit] = struct{}{}
			_, _ = fmt.Fprintf(&buf, "# %s %s => %s %s\n", r.Explicit.Path, r.Explicit.Version, m.Path, m.Version)
		} else {
			_, _ = fmt.Fprintf(&buf, "# %s %s\n", m.Path, m.Version)
		}

		if m.GoVersion != "" {
			_, _ = fmt.Fprintf(&buf, "## explicit; go %s\n", m.GoVersion)
		} else {
			_, _ = fmt.Fprintln(&buf, "## explicit")
		}

		for _, pkg := range m.Packages {
			_, _ = fmt.Fprintln(&buf, pkg)
		}
	}

	var finalReplaces []Replace
	for _, r := range replaces {
		if _, ok := replExcludes[r.Old]; ok {
			continue
		}

		finalReplaces = append(finalReplaces, r)
	}

	sort.Slice(finalReplaces, func(i, j int) bool {
		switch strings.Compare(finalReplaces[i].Old.Path, finalReplaces[j].Old.Path) {
		case -1:
			return true
		case 1:
			return false
		}

		return finalReplaces[i].Old.Version < finalReplaces[j].Old.Version
	})

	if len(finalReplaces) > 0 {
		_, _ = fmt.Fprintln(&buf, replacesMarker)
		for _, r := range finalReplaces {
			_, _ = fmt.Fprintf(&buf, "# %s => %s\n", encodeTXTModule(r.Old), encodeTXTModule(r.New))
		}
	}

	return buf.Bytes(), nil
}

func encodeTXTModule(m module.Version) string {
	if m.Version == "" {
		return m.Path
	}

	return m.Path + " " + m.Version
}

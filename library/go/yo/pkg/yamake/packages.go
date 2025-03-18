package yamake

import (
	"go/build"
	"sort"

	"a.yandex-team.ru/library/go/slices"
	"a.yandex-team.ru/library/go/yo/pkg/gomod"
)

type Target struct {
	GOARCH     string
	GOOS       string
	YaFlag     []string
	SubTargets []*Target
}

var AMD64 = &Target{
	GOARCH: "amd64",
	GOOS:   "any",
	YaFlag: []string{"ARCH_X86_64"},
	SubTargets: []*Target{
		LinuxAMD64,
		DarwinAMD64,
		WindowsAMD64,
	},
}

var ARM64 = &Target{
	GOARCH: "arm64",
	GOOS:   "any",
	YaFlag: []string{"ARCH_ARM64"},
	SubTargets: []*Target{
		LinuxARM64,
		DarwinARM64,
		WindowsARM64,
	},
}

var Linux = &Target{
	GOARCH: "any",
	GOOS:   "linux",
	YaFlag: []string{"OS_LINUX"},
	SubTargets: []*Target{
		LinuxAMD64,
		LinuxARM64,
	},
}

var LinuxAMD64 = &Target{
	GOARCH: "amd64",
	GOOS:   "linux",
	YaFlag: []string{"OS_LINUX", "AND", "ARCH_X86_64"},
}

var LinuxARM64 = &Target{
	GOARCH: "arm64",
	GOOS:   "linux",
	YaFlag: []string{"OS_LINUX", "AND", "ARCH_ARM64"},
}

var Darwin = &Target{
	GOARCH: "any",
	GOOS:   "darwin",
	YaFlag: []string{"OS_DARWIN"},
	SubTargets: []*Target{
		DarwinAMD64,
		DarwinARM64,
	},
}

var DarwinAMD64 = &Target{
	GOARCH: "amd64",
	GOOS:   "darwin",
	YaFlag: []string{"OS_DARWIN", "AND", "ARCH_X86_64"},
}

var DarwinARM64 = &Target{
	GOARCH: "arm64",
	GOOS:   "darwin",
	YaFlag: []string{"OS_DARWIN", "AND", "ARCH_ARM64"},
}

var Windows = &Target{
	GOARCH: "any",
	GOOS:   "windows",
	YaFlag: []string{"OS_WINDOWS"},
	SubTargets: []*Target{
		WindowsAMD64,
		// WindowsARM64 will be supported after Go 1.17: https://github.com/golang/go/issues/36439
		//WindowsARM64,
	},
}

var WindowsAMD64 = &Target{
	GOARCH: "amd64",
	GOOS:   "windows",
	YaFlag: []string{"OS_WINDOWS", "AND", "ARCH_X86_64"},
}

var WindowsARM64 = &Target{
	GOARCH: "arm64",
	GOOS:   "windows",
	YaFlag: []string{"OS_WINDOWS", "AND", "ARCH_ARM64"},
}
var Targets = []*Target{
	LinuxAMD64, LinuxARM64,
	DarwinAMD64, DarwinARM64,
	WindowsAMD64, //WindowsARM64,
}

var OpaqueTargets = []*Target{
	AMD64, ARM64,
	Linux, Darwin, Windows,
}

var AllTargets = []*Target{
	AMD64, ARM64,
	Linux, LinuxAMD64, LinuxARM64,
	Darwin, DarwinAMD64, DarwinARM64,
	Windows, WindowsAMD64, WindowsARM64,
}

type Sources struct {
	Files        []string
	CGoFiles     []string
	TestGoFiles  []string
	XTestGoFiles []string

	EmbedPatterns      []string
	TestEmbedPatterns  []string
	XTestEmbedPatterns []string
}

func (s *Sources) IsEmpty() bool {
	return len(s.Files) == 0 && len(s.CGoFiles) == 0 && len(s.TestGoFiles) == 0 && len(s.XTestGoFiles) == 0
}

type Package struct {
	Dir        string
	Name       string
	ImportPath string

	CommonSources Sources
	TargetSources map[*Target]*Sources
}

var Context = func() build.Context {
	context := build.Default
	context.GOROOT = ""
	return context
}()

type ImportError struct {
	Import string
	Error  error
}

func (p *Package) OnlyTests() bool {
	if !p.HasTests() {
		return false
	}

	onlyTests := func(sources *Sources) bool {
		return len(sources.Files) == 0 && len(sources.CGoFiles) == 0
	}

	if !onlyTests(&p.CommonSources) {
		return false
	}

	for _, s := range p.TargetSources {
		if !onlyTests(s) {
			return false
		}
	}

	return true
}

func (p *Package) HasTests() bool {
	if len(p.CommonSources.TestGoFiles) != 0 || len(p.CommonSources.XTestGoFiles) != 0 {
		return true
	}

	for _, s := range p.TargetSources {
		if len(s.TestGoFiles) != 0 || len(s.XTestGoFiles) != 0 {
			return true
		}
	}
	return false
}

const ArcadiaBuildTag = "arcadia"

// nolint: gocyclo
func Import(path string, srcDir string) (*Package, error) {
	p := &Package{TargetSources: make(map[*Target]*Sources)}

	files := make(map[string]int)
	cgoFiles := make(map[string]int)
	testFiles := make(map[string]int)
	xtestFiles := make(map[string]int)

	for _, target := range Targets {
		context := Context
		if context.GOROOT == "" {
			context.GOROOT = defaultGOROOT()
		}
		context.GOARCH = target.GOARCH
		context.GOOS = target.GOOS
		context.BuildTags = []string{ArcadiaBuildTag}
		context.Dir = srcDir

		pp, err := context.Import(path, srcDir, 0)
		if _, ok := err.(*build.NoGoError); ok {
			continue
		}
		if err != nil {
			return nil, err
		}

		p.Dir = pp.Dir
		p.Name = pp.Name
		p.ImportPath = pp.ImportPath

		sources := &Sources{}
		p.TargetSources[target] = sources

		addFile := func(name string) {
			files[name]++
			sources.Files = append(sources.Files, name)
		}

		for _, f := range pp.GoFiles {
			addFile(f)
		}
		for _, f := range pp.CFiles {
			addFile(f)
		}
		for _, f := range pp.CXXFiles {
			addFile(f)
		}
		for _, f := range pp.SFiles {
			addFile(f)
		}

		for _, f := range pp.CgoFiles {
			cgoFiles[f]++
			sources.CGoFiles = append(sources.CGoFiles, f)
		}

		for _, f := range pp.TestGoFiles {
			testFiles[f]++
			sources.TestGoFiles = append(sources.TestGoFiles, f)
		}

		for _, f := range pp.XTestGoFiles {
			xtestFiles[f]++
			sources.XTestGoFiles = append(sources.XTestGoFiles, f)
		}

		recordPatterns := func(at *[]string, patterns []string) {
			*at = append(*at, patterns...)
			*at = slices.DedupStrings(*at)
		}

		recordPatterns(&p.CommonSources.EmbedPatterns, pp.EmbedPatterns)
		recordPatterns(&p.CommonSources.TestEmbedPatterns, pp.TestEmbedPatterns)
		recordPatterns(&p.CommonSources.XTestEmbedPatterns, pp.XTestEmbedPatterns)
	}

	fillCommon := func(counts map[string]int, list func(s *Sources) *[]string) {
		var common []string
		for name, count := range counts {
			if count == len(Targets) {
				common = append(common, name)
			}
		}
		sort.Strings(common)
		*list(&p.CommonSources) = common

		for _, srcs := range p.TargetSources {
			var filtered []string
			for _, f := range *list(srcs) {
				if counts[f] != len(Targets) {
					filtered = append(filtered, f)
				}
			}
			sort.Strings(filtered)
			*list(srcs) = filtered
		}
	}

	fillOpaque := func(list func(s *Sources) *[]string) {
		for _, target := range OpaqueTargets {
			if len(target.SubTargets) == 0 {
				continue
			}

			var commonSrcs []string
			ok := true
			for i, subTarget := range target.SubTargets {
				subSources := p.TargetSources[subTarget]
				if subSources == nil {
					ok = false
					break
				}

				if i == 0 {
					srcs := *list(subSources)
					commonSrcs = srcs[:]
					continue
				}

				commonSrcs = slices.IntersectStrings(commonSrcs, *list(subSources))
			}

			if !ok {
				continue
			}

			if _, ok := p.TargetSources[target]; !ok {
				p.TargetSources[target] = &Sources{}
			}
			sort.Strings(commonSrcs)
			*list(p.TargetSources[target]) = commonSrcs

			excluded := make(map[string]struct{})
			for _, f := range commonSrcs {
				excluded[f] = struct{}{}
			}

			for _, subTarget := range target.SubTargets {
				subSources := p.TargetSources[subTarget]
				if subSources == nil {
					continue
				}

				var filtered []string
				for _, f := range *list(subSources) {
					if _, exclude := excluded[f]; exclude {
						continue
					}

					filtered = append(filtered, f)
				}

				sort.Strings(filtered)
				*list(subSources) = filtered
			}
		}
	}

	fill := func(counts map[string]int, list func(s *Sources) *[]string) {
		fillCommon(counts, list)
		fillOpaque(list)
	}

	fill(files, func(s *Sources) *[]string { return &s.Files })
	fill(cgoFiles, func(s *Sources) *[]string { return &s.CGoFiles })
	fill(testFiles, func(s *Sources) *[]string { return &s.TestGoFiles })
	fill(xtestFiles, func(s *Sources) *[]string { return &s.XTestGoFiles })

	for target, sources := range p.TargetSources {
		if sources.IsEmpty() {
			delete(p.TargetSources, target)
		}
	}

	return p, nil
}

func defaultGOROOT() string {
	goroot, _ := gomod.GoEnv("GOROOT")
	return goroot
}

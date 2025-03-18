package yamake

import (
	"bufio"
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"regexp"
	"strings"

	"github.com/google/go-cmp/cmp"
)

const (
	MacroOwner               = "OWNER"
	MacroLicense             = "LICENSE"
	MacroGoProgram           = "GO_PROGRAM"
	MacroGoLibrary           = "GO_LIBRARY"
	MacroProtoLibrary        = "PROTO_LIBRARY"
	MacroGoTest              = "GO_TEST"
	MacroGoTestFor           = "GO_TEST_FOR"
	MacroGoTestSrcs          = "GO_TEST_SRCS"
	MacroGoXTestSrcs         = "GO_XTEST_SRCS"
	MacroCgoSrcs             = "CGO_SRCS"
	MacroEnd                 = "END"
	MacroSrcs                = "SRCS"
	MacroTestCwd             = "TEST_CWD"
	MacroData                = "DATA"
	MacroIf                  = "IF"
	MacroElseIf              = "ELSEIF"
	MacroElse                = "ELSE"
	MacroEndIf               = "ENDIF"
	MacroAnd                 = "AND"
	MacroOr                  = "OR"
	MacroWith                = "WITH"
	MacroRecurse             = "RECURSE"
	MacroRecurseForTests     = "RECURSE_FOR_TESTS"
	MacroOsLinux             = "OS_LINUX"
	MacroOsDarwin            = "OS_DARWIN"
	MacroOsWindows           = "OS_WINDOWS"
	MacroArchARM64           = "ARCH_ARM64"
	MacroArchAMD64           = "ARCH_X86_64"
	MacroResource            = "RESOURCE"
	MacroGoEmbedPattern      = "GO_EMBED_PATTERN"
	MacroGoTestEmbedPattern  = "GO_TEST_EMBED_PATTERN"
	MacroGoXTestEmbedPattern = "GO_XTEST_EMBED_PATTERN"

	markIgnoreFile = "yo ignore:file"
)

type Macro struct {
	Name string
	Args []string

	IsComment    bool
	CommentLines []string
}

func (ya *YaMake) IsMarkedIgnore() bool {
	for _, m := range ya.Prefix {
		if !m.IsComment {
			continue
		}

		for _, line := range m.CommentLines {
			if strings.Contains(line, markIgnoreFile) {
				return true
			}
		}
	}

	return false
}

func (ya *YaMake) IsGoTestModule() bool {
	switch ya.Module.Name {
	case MacroGoTest, MacroGoTestFor:
		return true
	default:
		return false
	}
}

func (ya *YaMake) IsGoModule() bool {
	switch ya.Module.Name {
	case MacroGoLibrary, MacroGoProgram, MacroGoTest, MacroGoTestFor:
		return true
	default:
		return false
	}
}

type YaMake struct {
	// Part of ya.make that are not managed by the tool in
	// automated way and should be copied to output file as-is.
	Prefix, Suffix, Middle []Macro

	// Type of module, e.g GO_PROGRAM, GO_LIBRARY
	Module  Macro
	Owner   []string
	License []string

	CommonSources Sources
	TargetSources map[*Target]*Sources

	// Commented out files inside any of *SRCS macros. Used to preserve disabled tests inside vendor.
	DisabledFiles map[string]struct{}

	HasTestdata     bool
	TestdataDefined bool

	Recurse                []string
	RecurseForTests        []string
	TargetRecurse          map[*Target][]string
	CustomConditionRecurse map[string]struct{}
}

func (s *Sources) DisableFiles(disabledSet map[string]struct{}) {
	filter := func(list []string) {
		for i, file := range list {
			if _, filtered := disabledSet[file]; filtered {
				list[i] = "# " + file
			}
		}
	}

	filter(s.Files)
	filter(s.TestGoFiles)
	filter(s.XTestGoFiles)
	filter(s.CGoFiles)
}

func LoadYaMake(path string) (*YaMake, error) {
	f, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer func() { _ = f.Close() }()

	return ReadYaMake(f)
}

type yaTokenizer struct {
	scanner *bufio.Scanner
	tokens  []string
}

func newTokenizer(r io.Reader) *yaTokenizer {
	return &yaTokenizer{scanner: bufio.NewScanner(r)}
}

var tokenRe = regexp.MustCompile(`[^\t\n\f\r ()]+|\(|\)`)

func (t *yaTokenizer) scan() bool {
	if len(t.tokens) != 0 {
		t.tokens = t.tokens[1:]
	}

	for len(t.tokens) == 0 {
		if !t.scanner.Scan() {
			return false
		}

		var comment string
		lineBuffer := t.scanner.Text()
		commentIndex := strings.IndexByte(lineBuffer, '#')
		if commentIndex != -1 {
			comment = lineBuffer[commentIndex:]
			lineBuffer = lineBuffer[:commentIndex]
		}

		t.tokens = tokenRe.FindAllString(lineBuffer, -1)
		if comment != "" {
			t.tokens = append(t.tokens, strings.Trim(comment, " \n"))
		}
	}

	return true
}

func (t *yaTokenizer) undo() {
	tmp := t.tokens
	t.tokens = []string{""}
	t.tokens = append(t.tokens, tmp...)
}

func (t *yaTokenizer) token() string {
	if len(t.tokens) != 0 {
		return t.tokens[0]
	}

	return ""
}

var ErrYaMakeSyntax = fmt.Errorf("ya.make syntax error")

func (t *yaTokenizer) readMacro() (*Macro, error) {
	if !t.scan() {
		return nil, t.scanner.Err()
	}

	if t.token() == "(" || t.token() == ")" {
		return nil, ErrYaMakeSyntax
	}

	if t.token()[0] != '#' {
		m := &Macro{}
		m.Name = t.token()

		if !t.scan() || t.token() != "(" {
			return nil, ErrYaMakeSyntax
		}

		for {
			if !t.scan() {
				return nil, ErrYaMakeSyntax
			}

			if t.token() == ")" {
				return m, nil
			}

			m.Args = append(m.Args, t.token())
		}
	} else {
		m := &Macro{IsComment: true}
		for {
			m.CommentLines = append(m.CommentLines, t.token())
			if !t.scan() || t.token()[0] != '#' {
				t.undo()
				return m, nil
			}
		}
	}
}

func RemoveEmptyIfs(m []Macro) (filtered []Macro) {
	var empty = true
	var buffer []Macro

	for _, i := range m {
		switch {
		case i.Name == MacroIf || i.Name == MacroElse || i.Name == MacroElseIf:
			buffer = append(buffer, i)
		case i.Name == MacroEndIf:
			buffer = append(buffer, i)
			if !empty {
				filtered = append(filtered, buffer...)
			}

			buffer = nil
			empty = true
		case len(buffer) != 0:
			buffer = append(buffer, i)
			empty = false

		default:
			filtered = append(filtered, i)
		}
	}

	return
}

func NewYaMake() *YaMake {
	return &YaMake{
		TargetSources:          make(map[*Target]*Sources),
		TargetRecurse:          make(map[*Target][]string),
		DisabledFiles:          make(map[string]struct{}),
		CustomConditionRecurse: make(map[string]struct{}),
	}
}

// nolint: gocyclo
func ReadYaMake(r io.Reader) (*YaMake, error) {
	ya := NewYaMake()

	where := &ya.Prefix
	level := 0
	tok := newTokenizer(r)
	var target *Target

	for {
		m, err := tok.readMacro()
		if err != nil {
			return nil, err
		}

		if m == nil {
			break
		}

		switch m.Name {
		case MacroOwner:
			ya.Owner = m.Args
		case MacroLicense:
			ya.License = m.Args
		case MacroGoProgram, MacroGoLibrary, MacroGoTestFor, MacroGoTest, MacroProtoLibrary:
			ya.Module = *m
			where = &ya.Middle
		case MacroEnd:
			if where == &ya.Middle {
				where = &ya.Suffix
			} else {
				*where = append(*where, *m)
			}
		case MacroGoEmbedPattern, MacroGoTestEmbedPattern, MacroGoXTestEmbedPattern:

		case MacroSrcs, MacroGoTestSrcs, MacroGoXTestSrcs, MacroCgoSrcs:
			switch ya.Module.Name {
			case MacroGoProgram, MacroGoLibrary, MacroGoTest, MacroGoTestFor:
				for _, file := range m.Args {
					if !strings.HasPrefix(file, "#") {
						continue
					}

					realFilename := strings.TrimLeft(file, "# ")
					ya.DisabledFiles[realFilename] = struct{}{}
				}
			default:
				*where = append(*where, *m)
			}

		case MacroIf:
			level++
			*where = append(*where, *m)

			type targetPair struct {
				a1 string
				a2 string
			}

			var rawTarget targetPair
			switch len(m.Args) {
			case 1:
				rawTarget.a1 = m.Args[0]
			case 3:
				if m.Args[1] != MacroAnd {
					break
				}

				rawTarget.a1 = m.Args[0]
				rawTarget.a2 = m.Args[2]
			}

			switch rawTarget {
			case targetPair{a1: MacroOsLinux}:
				target = Linux
			case targetPair{a1: MacroOsLinux, a2: MacroArchAMD64}:
				target = LinuxAMD64
			case targetPair{a1: MacroOsLinux, a2: MacroArchARM64}:
				target = LinuxARM64
			case targetPair{a1: MacroOsDarwin}:
				target = Darwin
			case targetPair{a1: MacroOsDarwin, a2: MacroArchAMD64}:
				target = DarwinAMD64
			case targetPair{a1: MacroOsDarwin, a2: MacroArchARM64}:
				target = DarwinARM64
			case targetPair{a1: MacroOsWindows}:
				target = Windows
			case targetPair{a1: MacroOsWindows, a2: MacroArchAMD64}:
				target = WindowsAMD64
			case targetPair{a1: MacroOsWindows, a2: MacroArchARM64}:
				target = WindowsARM64
			case targetPair{a1: MacroArchARM64}:
				target = ARM64
			case targetPair{a1: MacroArchAMD64}:
				target = AMD64
			}

		case MacroEndIf:
			level--
			*where = append(*where, *m)
			target = nil

		case MacroRecurse, MacroRecurseForTests:
			if level == 0 {
				if m.Name == MacroRecurse {
					ya.Recurse = append(ya.Recurse, m.Args...)
				} else {
					ya.RecurseForTests = append(ya.RecurseForTests, m.Args...)
				}
			} else {
				if target != nil {
					ya.TargetRecurse[target] = append(ya.TargetRecurse[target], m.Args...)
				} else {
					for _, arg := range m.Args {
						ya.CustomConditionRecurse[arg] = struct{}{}
					}
					*where = append(*where, *m)
				}
			}

		default:
			if m.Name == MacroTestCwd || m.Name == MacroData {
				ya.TestdataDefined = true
			}

			*where = append(*where, *m)

			if m.Name == MacroElse || m.Name == MacroElseIf {
				target = nil
			}
		}
	}

	ya.Prefix = RemoveEmptyIfs(ya.Prefix)
	ya.Middle = RemoveEmptyIfs(ya.Middle)
	ya.Suffix = RemoveEmptyIfs(ya.Suffix)

	return ya, nil
}

func isIfOpen(name string) bool {
	return name == MacroIf || name == MacroElseIf || name == MacroElse
}

func isIfClose(name string) bool {
	return name == MacroElseIf || name == MacroElse || name == MacroEndIf
}

func markWithCGOExport(files []string) (out []string) {
	for _, f := range files {
		switch {
		case strings.HasPrefix(f, "#"):
			out = append(out, f)

		case strings.HasSuffix(f, ".cpp") || strings.HasSuffix(f, ".c"):
			out = append(out, "CGO_EXPORT", f)

		default:
			out = append(out, f)
		}
	}

	return
}

// nolint: gocyclo
func (ya *YaMake) WriteResult(w io.Writer) {
	addNewline := false
	level := 0
	writeMacro := func(m Macro) {
		if addNewline && !isIfClose(m.Name) {
			_, _ = fmt.Fprintf(w, "\n")
		}
		addNewline = true

		if isIfClose(m.Name) {
			level--
		}

		prefix := strings.Repeat(" ", level*4)

		if isIfOpen(m.Name) {
			level++
			addNewline = false
		}

		if m.Name == MacroIf {
			m.Name += " "
		}

		if m.IsComment {
			for _, line := range m.CommentLines {
				_, _ = fmt.Fprintf(w, "%s%s\n", prefix, line)
			}
		} else if len(m.Args) == 0 {
			_, _ = fmt.Fprintf(w, "%s%s()\n", prefix, m.Name)
		} else if len(m.Args) == 1 && m.Args[0][0] != '#' {
			_, _ = fmt.Fprintf(w, "%s%s(%s)\n", prefix, m.Name, m.Args[0])
		} else {
			_, _ = fmt.Fprintf(w, "%s%s(\n", prefix, m.Name)
			if m.Name == MacroResource && len(m.Args)%2 == 0 {
				for i := 0; 2*i < len(m.Args); i++ {
					_, _ = fmt.Fprintf(w, "%s    %s %s\n", prefix, m.Args[2*i], m.Args[2*i+1])
				}
			} else {
				for _, arg := range m.Args {
					_, _ = fmt.Fprintf(w, "%s    %s\n", prefix, arg)
				}
			}
			_, _ = fmt.Fprintf(w, "%s)\n", prefix)
		}
	}

	writeMacroLicense := func(licenses []string) {
		isPreviousOperator := true
		isWithLicense := false
		if addNewline {
			_, _ = fmt.Fprintf(w, "\n")
		}
		addNewline = true

		prefix := strings.Repeat(" ", level*4)

		if len(licenses) == 0 {
			_, _ = fmt.Fprintf(w, "%s%s()\n", prefix, MacroLicense)
		} else if len(licenses) == 1 && licenses[0][0] != '#' {
			_, _ = fmt.Fprintf(w, "%s%s(%s)\n", prefix, MacroLicense, licenses[0])
		} else {
			_, _ = fmt.Fprintf(w, "%s%s(\n", prefix, MacroLicense)
			for i := 0; i < len(licenses); i++ {
				arg := licenses[i]
				if arg == MacroAnd || arg == MacroOr {
					isPreviousOperator = true
					isWithLicense = false
					_, _ = fmt.Fprintf(w, " %s\n", arg)
					continue
				}

				if arg == MacroWith {
					isPreviousOperator = false
					isWithLicense = true
					_, _ = fmt.Fprintf(w, " %s", arg)
					continue
				}

				if isWithLicense {
					isWithLicense = false
					isPreviousOperator = false
					_, _ = fmt.Fprintf(w, " %s", arg)
					continue
				}

				if isPreviousOperator {
					isWithLicense = false
					isPreviousOperator = false
					_, _ = fmt.Fprintf(w, "%s    %s", prefix, arg)
					continue
				}

				isPreviousOperator = false
				isWithLicense = false
				_, _ = fmt.Fprintf(w, " %s\n%s    %s", MacroAnd, prefix, arg)
			}

			_, _ = fmt.Fprintf(w, "\n%s)\n", prefix)
		}
	}

	if ya.Module.Name == "" && len(ya.Owner) != 0 {
		writeMacro(Macro{Name: MacroOwner, Args: ya.Owner})
	}

	for _, m := range ya.Prefix {
		writeMacro(m)
	}

	if ya.Module.Name != "" {
		writeMacro(ya.Module)

		if len(ya.Owner) != 0 {
			writeMacro(Macro{Name: MacroOwner, Args: ya.Owner})
		}

		if len(ya.License) != 0 {
			writeMacroLicense(ya.License)
		}

		for _, m := range ya.Middle {
			writeMacro(m)
		}

		writeSrcs := func(s *Sources) {
			if len(s.Files) != 0 {
				writeMacro(Macro{Name: MacroSrcs, Args: markWithCGOExport(s.Files)})
			}

			if len(s.CGoFiles) != 0 {
				writeMacro(Macro{Name: MacroCgoSrcs, Args: s.CGoFiles})
			}

			if len(s.TestGoFiles) != 0 {
				writeMacro(Macro{Name: MacroGoTestSrcs, Args: s.TestGoFiles})
			}

			if len(s.XTestGoFiles) != 0 {
				writeMacro(Macro{Name: MacroGoXTestSrcs, Args: s.XTestGoFiles})
			}
		}

		writeSrcs(&ya.CommonSources)
		for _, t := range AllTargets {
			s, ok := ya.TargetSources[t]
			if !ok {
				continue
			}

			writeMacro(Macro{Name: MacroIf, Args: t.YaFlag})

			writeSrcs(s)

			writeMacro(Macro{Name: MacroEndIf})
		}

		writePattern := func(name string, patterns []string) {
			for _, p := range patterns {
				writeMacro(Macro{
					Name: name,
					Args: []string{p},
				})
			}
		}

		writePattern(MacroGoEmbedPattern, ya.CommonSources.EmbedPatterns)
		writePattern(MacroGoTestEmbedPattern, ya.CommonSources.TestEmbedPatterns)
		writePattern(MacroGoXTestEmbedPattern, ya.CommonSources.XTestEmbedPatterns)

		if ya.HasTestdata && !ya.TestdataDefined {
			testFor := ya.Module.Args[0]
			writeMacro(Macro{Name: MacroData, Args: []string{path.Join("arcadia", testFor, "testdata")}})
			writeMacro(Macro{Name: MacroTestCwd, Args: []string{testFor}})
		}

		writeMacro(Macro{Name: MacroEnd})
	}

	if len(ya.Recurse) != 0 {
		writeMacro(Macro{Name: MacroRecurse, Args: ya.Recurse})
	}

	if len(ya.RecurseForTests) != 0 {
		writeMacro(Macro{Name: MacroRecurseForTests, Args: ya.RecurseForTests})
	}

	for _, target := range AllTargets {
		recurse := ya.TargetRecurse[target]
		if len(recurse) > 0 {
			writeMacro(Macro{Name: MacroIf, Args: target.YaFlag})
			writeMacro(Macro{Name: MacroRecurse, Args: recurse})
			writeMacro(Macro{Name: MacroEndIf})
		}
	}

	for _, m := range ya.Suffix {
		writeMacro(m)
	}
}

func (ya *YaMake) Save(path string) error {
	var b bytes.Buffer
	ya.WriteResult(&b)
	return ioutil.WriteFile(path, b.Bytes(), 0644)
}

func (ya *YaMake) RemoveEnabledRecurses() {
	clear := func(in []string) (out []string) {
		for _, r := range in {
			if len(r) != 0 && r[0] == '#' {
				out = append(out, r)
			}
		}
		return
	}

	ya.Recurse = clear(ya.Recurse)
	ya.RecurseForTests = clear(ya.RecurseForTests)

	// Do not touch target specific RECURSE.
}

func (ya *YaMake) DisableFiles() {
	ya.CommonSources.DisableFiles(ya.DisabledFiles)
	for _, target := range ya.TargetSources {
		target.DisableFiles(ya.DisabledFiles)
	}
}

func LoadYaMakeFiles(yaMakes map[string]*YaMake, arcadiaRoot, arcadiaPath string) error {
	walkRoot := filepath.Join(arcadiaRoot, arcadiaPath)

	if _, err := os.Stat(walkRoot); os.IsNotExist(err) {
		return nil
	}

	return filepath.Walk(walkRoot, func(filePath string, info os.FileInfo, err error) error {
		if err != nil {
			return nil
		}

		if info.Name() == "ya.make" {
			arcadiaPath := path.Dir(filepath.ToSlash(filePath))
			arcadiaPath = arcadiaPath[len(arcadiaRoot)+1:]

			yaMakes[arcadiaPath], err = LoadYaMake(filePath)
			if err != nil {
				return err
			}
		}
		return nil
	})
}

func SaveYaMakeFiles(arcadiaRoot string, yaMakes map[string]*YaMake) error {
	for arcadiaPath, yaMake := range yaMakes {
		p := arcadiaRoot + "/" + arcadiaPath + "/ya.make"
		if err := os.MkdirAll(filepath.Dir(p), 0755); err != nil {
			return err
		}

		if err := yaMake.Save(p); err != nil {
			return err
		}
	}

	return nil
}

func DiffYaMakeFiles(arcadiaRoot string, yaMakes map[string]*YaMake) (map[string]string, error) {
	diff := make(map[string]string)
	for arcadiaPath, yaMake := range yaMakes {
		var newBuf bytes.Buffer
		yaMake.WriteResult(&newBuf)
		newStr := newBuf.String()

		p := arcadiaRoot + "/" + arcadiaPath + "/ya.make"

		old, err := readFile(p)
		if err != nil {
			return nil, err
		}

		var oldStr string
		if old != nil {
			oldStr = string(old)
		}

		if oldStr == newStr {
			continue
		}

		diff[arcadiaPath+"/ya.make"] = cmp.Diff(oldStr, newStr)
	}

	return diff, nil
}

func readFile(p string) ([]byte, error) {
	if _, err := os.Stat(p); err != nil {
		if os.IsNotExist(err) {
			return nil, nil
		}

		return nil, err
	}

	f, err := os.Open(p)
	if err != nil {
		return nil, err
	}
	defer func() { _ = f.Close() }()

	return ioutil.ReadAll(f)
}

func AddOwner(yaMakes map[string]*YaMake, owner string) {
nextYaMake:
	for _, yaMake := range yaMakes {
		for _, o := range yaMake.Owner {
			if o == owner {
				continue nextYaMake
			}
		}

		yaMake.Owner = append(yaMake.Owner, owner)
	}
}

func SetLicense(yaMakes map[string]*YaMake, license []string) {
	for _, yaMake := range yaMakes {
		yaMake.License = license
	}
}

func InferDefaultOwner(yaMakes map[string]*YaMake) {
	for arcadiaPath, yaMake := range yaMakes {
		if len(yaMake.Owner) != 0 {
			continue
		}

		parent := arcadiaPath
		for {
			parent = path.Dir(parent)
			if parent == "." {
				break
			}

			parentYaMake := yaMakes[parent]

			if parentYaMake == nil {
				break
			}

			if len(parentYaMake.Owner) != 0 {
				yaMake.Owner = append([]string{}, parentYaMake.Owner...)
				break
			}
		}
	}
}

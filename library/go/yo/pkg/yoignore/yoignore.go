package yoignore

import (
	"io/ioutil"
	"os"
	"path"
	"path/filepath"
	"strings"
)

type rules struct {
	globs []string
}

type Ignore struct {
	dirs map[string]rules
}

const Filename = ".yoignore"

func Load(arcadiaRoot string, path string) (*Ignore, error) {
	ignore := &Ignore{
		dirs: make(map[string]rules),
	}

	err := filepath.Walk(filepath.Join(arcadiaRoot, path), func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		if !info.IsDir() && info.Name() == Filename {
			if err := ignore.loadRule(arcadiaRoot, path); err != nil {
				return err
			}
		}

		return nil
	})

	if err != nil && !os.IsNotExist(err) {
		return nil, err
	}

	for {
		path = filepath.Dir(path)

		err := ignore.loadRule(arcadiaRoot, filepath.Join(arcadiaRoot, path, Filename))
		if err != nil && !os.IsNotExist(err) {
			return nil, err
		}

		if path == "." {
			break
		}
	}

	return ignore, nil
}

func (i *Ignore) loadRule(arcadiaRoot, path string) error {
	rel, err := filepath.Rel(arcadiaRoot, path)
	if err != nil {
		return err
	}
	dir := filepath.ToSlash(filepath.Dir(rel))

	rulesBytes, err := ioutil.ReadFile(path)
	if err != nil {
		return err
	}

	globs := strings.Split(string(rulesBytes), "\n")
	i.dirs[dir] = rules{globs: globs}
	return nil
}

func (i *Ignore) Save(arcadiaRoot string) error {
	for dir, rules := range i.dirs {
		rulesStr := strings.TrimSpace(strings.Join(rules.globs, "\n")) + "\n"

		if err := os.MkdirAll(filepath.Join(arcadiaRoot, dir), 0777); err != nil {
			return err
		}

		ignorePath := filepath.Join(arcadiaRoot, dir, Filename)
		if err := ioutil.WriteFile(ignorePath, []byte(rulesStr), 0666); err != nil {
			return err
		}
	}

	return nil
}

func isRule(rule string) bool {
	return rule != "" && rule[0] != '#'
}

func isRelative(rule string) bool {
	i := strings.IndexByte(rule, '/')
	return i != -1 && i+1 < len(rule)
}

func isDirRule(rule string) bool {
	return len(rule) > 0 && rule[len(rule)-1] == '/'
}

func (i *Ignore) Ignores(targetPath string, isDir bool) bool {
	dir := path.Dir(targetPath)
	names := []string{path.Base(targetPath)}

	for dir != "." {
		rules := i.dirs[dir]

		for _, glob := range rules.globs {
			if !isRule(glob) {
				continue
			}

			if !isDir && isDirRule(glob) {
				continue
			}

			if isDirRule(glob) {
				glob = glob[:len(glob)-1]
			}

			if isRelative(glob) {
				if glob[0] == '/' {
					glob = glob[1:]
				}

				if matched, _ := path.Match(glob, names[len(names)-1]); matched {
					return true
				}
			} else {
				for _, nameInDir := range names {
					if matched, _ := path.Match(glob, nameInDir); matched {
						return true
					}
				}
			}
		}

		names = append(names, path.Join(path.Base(dir), names[len(names)-1]))
		dir = path.Dir(dir)
	}

	return false
}

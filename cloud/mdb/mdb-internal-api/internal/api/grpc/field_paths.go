package grpc

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type FieldPaths struct {
	paths  []string
	prefix string
	root   *FieldPaths
}

var allPaths FieldPaths

func NewFieldPaths(paths []string) *FieldPaths {
	fieldPaths := &FieldPaths{paths: paths}
	fieldPaths.root = fieldPaths
	return fieldPaths
}

func AllPaths() *FieldPaths {
	return &allPaths
}

func (p *FieldPaths) Empty() bool {
	if p == &allPaths {
		return false
	}
	for _, path := range p.root.paths {
		if strings.HasPrefix(path, p.prefix) {
			return false
		}
	}
	return true
}

func (p *FieldPaths) Remove(pathToRemove string) bool {
	if p == &allPaths {
		return true
	}
	if p != p.root {
		return p.root.Remove(p.prefix + pathToRemove)
	}

	newPaths := make([]string, 0, len(p.paths))
	found := false
	for _, path := range p.paths {
		if path == pathToRemove {
			found = true
		} else {
			newPaths = append(newPaths, path)
		}
	}
	p.paths = newPaths
	return found
}

func (p *FieldPaths) Subtree(prefix string) *FieldPaths {
	if p == &allPaths {
		return &allPaths
	}
	return &FieldPaths{
		paths:  nil,
		prefix: p.prefix + prefix,
		root:   p.root,
	}
}

func (p *FieldPaths) SubPaths() []string {
	var result []string
	prefixLength := len(p.prefix)
	for _, path := range p.root.paths {
		if strings.HasPrefix(path, p.prefix) {
			subPath := path[prefixLength:]
			if len(subPath) > 0 {
				result = append(result, subPath)
			}
		}
	}
	return result
}

func (p *FieldPaths) MustBeEmpty() error {
	if p == &allPaths {
		return nil
	}
	if p != p.root {
		panic("FieldPaths.MustBeEmpty should be called for root FieldPaths object only")
	}

	if len(p.paths) != 0 {
		invalidPaths := strings.Join(p.paths, ", ")
		return semerr.InvalidInputf("unknown field paths: %s", invalidPaths)
	}
	return nil
}

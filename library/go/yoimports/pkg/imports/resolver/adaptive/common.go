package adaptive

import (
	"errors"
	"fmt"
	"go/parser"
	"go/token"
	"os"
	"path"
	"path/filepath"
	"sort"
	"strings"

	"github.com/jhump/protoreflect/desc/protoparse"
)

// Inspired by goimports.
func packageDirToName(dir, importPath string, parseProto bool) (string, error) {
	d, err := os.Open(dir)
	if err != nil {
		return "", err
	}

	names, err := d.Readdirnames(-1)
	_ = d.Close()
	if err != nil {
		return "", err
	}

	sort.Strings(names) // to have predictable behavior
	var (
		lastErr error
		nFile   int
		pkgName string
	)

	for _, name := range names {
		switch {
		case strings.HasSuffix(name, ".go") && !strings.HasSuffix(name, "_test.go"):
			fullFile := filepath.Join(dir, name)
			pkgName, lastErr = goFileToName(fullFile)
		case parseProto && strings.HasSuffix(name, ".proto"):
			fullFile := filepath.Join(dir, name)
			pkgName, lastErr = protoFileToName(fullFile, importPath)
		default:
			continue
		}

		if pkgName != "" {
			return pkgName, nil
		}
		nFile++
	}

	if lastErr != nil {
		return "", lastErr
	}
	return "", fmt.Errorf("no importable package found in %d Go files", nFile)
}

func goFileToName(fullFile string) (string, error) {
	fset := token.NewFileSet()
	f, err := parser.ParseFile(fset, fullFile, nil, parser.PackageClauseOnly)
	if err != nil {
		return "", err
	}

	pkgName := f.Name.Name
	if pkgName == "documentation" {
		// Special case from go/build.ImportDir
		return "", errors.New("doc pkg")
	}

	if pkgName == "main" {
		// Also skip package main, assuming it's a +build ignore generator or example.
		// Since you can't import a package main anyway, there's no harm here.
		return "", errors.New("main pkg")
	}

	return pkgName, nil
}

func protoFileToName(fullFile, importPath string) (string, error) {
	pp := protoparse.Parser{
		InterpretOptionsInUnlinkedFiles: true,
	}
	files, err := pp.ParseFilesButDoNotLink(fullFile)
	if err != nil {
		return "", err
	}

	if len(files) != 1 {
		return "", fmt.Errorf("unexpected protoparse result: %d", len(files))
	}

	if files[0].Options == nil || files[0].Options.GoPackage == nil {
		return "", errors.New("no go_package option")
	}

	parts := strings.Split(*files[0].Options.GoPackage, ";")
	var protoImportPath, pkgName string
	switch len(parts) {
	case 1:
		protoImportPath = parts[0]
	case 2:
		protoImportPath, pkgName = parts[0], parts[1]
	default:
		return "", fmt.Errorf("unexpected go_package option: %q", *files[0].Options.GoPackage)
	}

	if protoImportPath != importPath {
		return "", fmt.Errorf("unexpected import path in the go_package option: %s", protoImportPath)
	}

	if pkgName != "" {
		return pkgName, nil
	}

	return path.Base(protoImportPath), nil
}

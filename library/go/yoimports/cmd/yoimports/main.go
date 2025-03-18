package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"a.yandex-team.ru/library/go/core/buildinfo"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/adaptive"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/cached"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/golist"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver/gopackages"
)

var (
	flagWriteToFile bool
	flagWithGen     bool
	flagFormatOnly  bool
	flagLocalPrefix = imports.DefaultLocalPrefix
	importsResolver = newImportsResolver()
)

const usage = `yoimports is an opinionated tool for Go imports formatting.

Usage: yoimports [flags] [path ...|-]
`

func parseFlags() {
	flag.BoolVar(&flagWriteToFile, "w", false, "write changes to file instead of stdout")
	flag.BoolVar(&flagWithGen, "with-gen", false, "process auto-generated files")
	flag.BoolVar(&flagFormatOnly, "format-only", false, "if true, don't fix imports and only format")
	flag.StringVar(&flagLocalPrefix, "local", flagLocalPrefix, "use specified import prefix as 'local' import path when grouping imports")
	printVersion := flag.Bool("v", false, "print version")
	flag.Parse()

	flag.Usage = func() {
		fmt.Print(usage)
	}

	if *printVersion {
		fmt.Print(buildinfo.Info.ProgramVersion)
		os.Exit(2)
	}
}

func fatalf(format string, args ...interface{}) {
	_, _ = fmt.Fprintf(os.Stderr, "yoimports: "+format+"\n", args...)
	os.Exit(1)
}

func main() {
	parseFlags()

	paths := flag.Args()
	if len(paths) == 0 {
		flag.Usage()
		flag.PrintDefaults()
		os.Exit(2)
	}

	if len(paths) == 1 && paths[0] == "-" {
		if err := processStream(os.Stdout, os.Stdin); err != nil {
			fatalf("error while processing stdin: %s", err)
		}
		return
	}

	if err := processPaths(paths); err != nil {
		fatalf("error while processing paths: %s", err)
	}
}

func processStream(w io.Writer, r io.Reader) error {
	in, err := ioutil.ReadAll(r)
	if err != nil {
		return err
	}

	formatted, err := processBytes(in)
	if err != nil {
		return err
	}

	_, err = w.Write(formatted)
	return err
}

func processPaths(paths []string) error {
	cwd, err := os.Getwd()
	if err != nil {
		return fmt.Errorf("unable to get current working dir: %w", err)
	}

	for _, path := range paths {
		if !filepath.IsAbs(path) {
			path = filepath.Join(cwd, path)
		}
		path = filepath.Clean(path)

		if isVendored(path) {
			// skip vendored files
			continue
		}

		switch dir, err := os.Stat(path); {
		case err != nil:
			fatalf("cannot stat path %s: %s", path, err)
		case dir.IsDir():
			err := filepath.Walk(path, visitFile)
			if err != nil {
				return fmt.Errorf("unexpected error while traversing path %s: %w", path, err)
			}
		default:
			if !isGoFile(dir) {
				continue
			}

			source, err := processFile(path)
			if err != nil {
				return fmt.Errorf("error while processing file %s: %w", path, err)
			}
			err = writeSource(path, source)
			if err != nil {
				return fmt.Errorf("error while writing source of %s: %w", path, err)
			}
		}
	}

	return nil
}

func visitFile(path string, f os.FileInfo, err error) error {
	if err != nil {
		return err
	}

	if !isGoFile(f) {
		return nil
	}

	source, err := processFile(path)
	if err != nil {
		return fmt.Errorf("error while processing file %s: %s", path, err)
	}

	if source != nil {
		return writeSource(path, source)
	}
	return nil
}

func processFile(path string) ([]byte, error) {
	if isVendored(path) {
		// skip vendored files
		return nil, nil
	}

	originalSource, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, err
	}

	formattedSource, err := processBytes(originalSource)
	if err != nil {
		return nil, err
	}

	// return nothing if nothing changed
	if bytes.Equal(originalSource, formattedSource) {
		return nil, nil
	}

	return formattedSource, nil
}

func processBytes(source []byte) ([]byte, error) {
	return imports.Process(source,
		imports.WithLocalPrefix(flagLocalPrefix),
		imports.WithGenerated(flagWithGen),
		imports.WithOptimizeImports(!flagFormatOnly),
		imports.WithImportsResolver(importsResolver),
	)
}

func writeSource(path string, source []byte) error {
	if source == nil {
		return nil
	}

	// write result to stdout
	if !flagWriteToFile {
		fmt.Printf("%s\n========\n%s\n\n", path, source)
		return nil
	}

	// write result to original file
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	defer f.Close()

	_, err = f.Write(source)
	return err
}

func isGoFile(f os.FileInfo) bool {
	name := f.Name()
	return f.Mode().IsRegular() && !strings.HasPrefix(name, ".") && strings.HasSuffix(name, ".go")
}

func isVendored(path string) bool {
	sep := string(filepath.Separator)
	return strings.HasPrefix(path, "vendor"+sep) ||
		strings.Contains(path, sep+"vendor"+sep)
}

func newImportsResolver() resolver.Resolver {
	var r resolver.Resolver
	env := os.Getenv("YOIMPORTS_RESOLVER")
	switch env {
	case "adaptive", "":
		r = adaptive.NewResolver()
	case "golist":
		r = golist.NewResolver()
	case "gopackages":
		r = gopackages.NewResolver()
	default:
		fatalf("unsupported imports resolver: %s", env)
		return nil
	}

	return cached.NewResolver(r)
}

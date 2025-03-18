package imports

import (
	"go/ast"
	"go/parser"
	"go/token"
	"log"
	"sort"
	"strings"

	"github.com/dave/dst"
	"github.com/dave/dst/decorator"

	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/yoimports/pkg/imports/resolver"
	"a.yandex-team.ru/library/go/yolint/pkg/lintutils"
)

const (
	impGroupSTD      = 0
	impGroup3rdParty = 1
	impGroupLocal    = 2
	impGroupMax      = impGroupLocal
)

type formatter struct {
	localPrefix  string
	resolver     resolver.Resolver
	removeUnused bool
	processGen   bool
}

// Process formats source code in canonical gofmt style and returns result.
// Due to buggy github.com/dave/dst parser we're doing out things a bit tricky:
//   1. Format code with a standard printer
//   2. Parse resulted output again to the last import declaration
//   3. Format the "header" (package/imports decls) with github.com/dave/dst
//   4. Concatenate "header" and the rest of code
//
//This way dave/dst has minimal effect on the resulting code, so we are closer to gofmt than ever :)
func (f *formatter) Process(src []byte) ([]byte, error) {
	// first, format source code with a standard printer
	fileSet := token.NewFileSet()
	astFile, err := parser.ParseFile(fileSet, "", src, parser.ParseComments)
	if err != nil {
		return nil, xerrors.Errorf("can't parse source code: %w", err)
	}

	if !f.processGen && lintutils.IsGenerated(astFile) {
		return src, nil
	}

	src, err = printAstFile(fileSet, astFile)
	if err != nil {
		return nil, err
	}

	if len(astFile.Imports) == 0 {
		return src, nil
	}

	// now parse it again to the last import declaration
	offset, err := bodyOffset(src)
	if err != nil {
		return nil, xerrors.Errorf("can't determine body offset: %w", err)
	}

	fileSet = token.NewFileSet()
	astHeader, err := parser.ParseFile(fileSet, "", src[:offset], parser.ParseComments)
	if err != nil {
		return nil, xerrors.Errorf("can't parse formatted source code: %w", err)
	}

	dstFile, err := decorator.NewDecorator(fileSet).DecorateFile(astHeader)
	if err != nil {
		return nil, xerrors.Errorf("can't decorate code with dst: %w", err)
	}

	// doing some imports normalizing magic
	if f.removeUnused {
		f.removeUnusedImports(dstFile, astFile)
	}
	f.mergeImports(dstFile)
	f.sortImports(dstFile)

	// format it
	out, err := printDstFile(dstFile)
	if err != nil {
		return nil, err
	}

	if offset == len(src) {
		// imports only
		return out, nil
	}

	return append(out, src[offset+1:]...), err
}

func (f *formatter) removeUnusedImports(dstFile *dst.File, astFile *ast.File) {
	if len(dstFile.Decls) == 0 {
		return
	}

	for i := 0; i < len(dstFile.Decls); i++ {
		decl := dstFile.Decls[i]
		gen, ok := decl.(*dst.GenDecl)
		if !ok || gen.Tok != token.IMPORT || declImports(gen, "C") {
			continue
		}

		gen.Specs = f.removeUnusedSpecs(astFile, gen.Specs)
	}
}

func (f *formatter) removeUnusedSpecs(astFile *ast.File, in []dst.Spec) []dst.Spec {
	type usedSpec struct {
		used bool
		name string
		path string
		spec *dst.ImportSpec
	}

	collectSpecs := func(in []dst.Spec) []*usedSpec {
		specs := make([]*usedSpec, len(in))
		for i, genSpec := range in {
			spec := genSpec.(*dst.ImportSpec)
			specs[i] = &usedSpec{
				used: false,
				name: importName(spec),
				path: importPath(spec),
				spec: spec,
			}
		}
		return specs
	}

	markUsedNames := func(specs []*usedSpec) {
		candidates := make(map[string][]*usedSpec, len(specs))
		for _, spec := range specs {
			if spec.used {
				continue
			}

			switch spec.name {
			case ".", "_":
				// dot/blank imports are always used
				spec.used = true
			case "":
				name := importPathToAssumedName(spec.path)
				candidates[name] = append(candidates[name], spec)
			default:
				candidates[spec.name] = append(candidates[spec.name], spec)
			}
		}

		if len(candidates) == 0 {
			return
		}

		ast.Inspect(astFile, func(n ast.Node) bool {
			sel, ok := n.(*ast.SelectorExpr)
			if !ok || sel.X == nil {
				return true
			}

			ident, ok := sel.X.(*ast.Ident)
			if !ok {
				return true
			}

			// it may be false positive with shadow names, but it is fast.
			if specs, ok := candidates[ident.Name]; ok {
				for _, spec := range specs {
					spec.used = true
				}
			}
			return true
		})
	}

	resolveImports := func(specs []*usedSpec) {
		var (
			candidates    []string
			candidatesPos []int
		)

		for i, spec := range specs {
			if spec.used {
				continue
			}

			if spec.name != "" {
				continue
			}

			if !strings.Contains(spec.path, ".") {
				// skip std imports
				continue
			}

			candidates = append(candidates, spec.path)
			candidatesPos = append(candidatesPos, i)
		}

		if len(candidates) == 0 || len(candidatesPos) == 0 {
			return
		}

		resolvedPkgs, err := f.resolver.ResolvePackages(candidates...)
		if err != nil {
			log.Printf("failed to resolve pkgs (assume all unresolved imports are used): %v\n", err)
			for _, specPos := range candidatesPos {
				specs[specPos].used = true
			}
			return
		}

		for i, pkg := range resolvedPkgs {
			spec := specs[candidatesPos[i]]

			if pkg.Name == "" {
				// if we can't resolve pkg - mark it as used. This is safest way, imho.
				spec.used = true
				continue
			}

			spec.name = pkg.Name
			if spec.spec.Name == nil {
				spec.spec.Name = &dst.Ident{
					Name: pkg.Name,
				}
			} else {
				spec.spec.Name.Name = pkg.Name
			}
		}
	}

	specs := collectSpecs(in)
	markUsedNames(specs)
	resolveImports(specs)
	markUsedNames(specs)

	out := make([]dst.Spec, 0, len(specs))
	for _, spec := range specs {
		if !spec.used {
			continue
		}

		out = append(out, spec.spec)
	}
	return out
}

// mergeImports merges all the import declarations into the first one.
// Inspired by golang.org/x/tools/ast/astutil.
func (f *formatter) mergeImports(file *dst.File) {
	if len(file.Decls) <= 1 {
		return
	}

	// Merge all the import declarations into the first one.
	var first *dst.GenDecl
	for i := 0; i < len(file.Decls); i++ {
		decl := file.Decls[i]
		gen, ok := decl.(*dst.GenDecl)
		if !ok || gen.Tok != token.IMPORT || declImports(gen, "C") {
			continue
		}

		if first == nil {
			first = gen
			continue // Don't touch the first one.
		}

		// Move the imports of the other import declaration to the first one.
		first.Specs = append(first.Specs, gen.Specs...)
		file.Decls = append(file.Decls[:i], file.Decls[i+1:]...)
		i--
	}
}

func (f *formatter) sortImports(file *dst.File) {
	for i := 0; i < len(file.Decls); i++ {
		decl := file.Decls[i]
		gen, ok := decl.(*dst.GenDecl)
		if !ok || gen.Tok != token.IMPORT {
			continue
		}

		if len(gen.Specs) == 0 {
			// Empty import block, remove it.
			file.Decls = append(file.Decls[:i], file.Decls[i+1:]...)
		}

		if len(gen.Specs) == 1 {
			// single import was sorted by default :)
			continue
		}

		gen.Specs = f.rearrangeSpecs(gen.Specs)
	}
}

func (f *formatter) rearrangeSpecs(in []dst.Spec) []dst.Spec {
	specs := make([]*dst.ImportSpec, len(in))
	for i := range in {
		specs[i] = in[i].(*dst.ImportSpec)
		if i == 0 {
			specs[i].Decs.Before = dst.None
		} else {
			specs[i].Decs.Before = dst.NewLine
		}

		specs[i].Decs.After = dst.None
	}

	var out []dst.Spec
	for groupNum, specs := range f.groupSpecs(specs) {
		if len(specs) == 0 {
			continue
		}

		if groupNum != impGroupMax {
			specs[len(specs)-1].Decs.After = dst.EmptyLine
		}

		for _, spec := range specs {
			out = append(out, spec)
		}
	}

	return out
}

func (f *formatter) groupSpecs(specs []*dst.ImportSpec) [][]*dst.ImportSpec {
	out := make([][]*dst.ImportSpec, impGroupMax+1)

	for _, spec := range specs {
		impGroup := f.importGroup(spec)
		out[impGroup] = append(out[impGroup], spec)
	}

	for outIdx := range out {
		curSpecs := out[outIdx]
		if len(curSpecs) == 0 {
			continue
		}

		sort.Slice(curSpecs, func(i, j int) bool {
			switch strings.Compare(curSpecs[i].Path.Value, curSpecs[j].Path.Value) {
			case -1:
				return true
			case 1:
				return false
			}

			return importName(curSpecs[i]) < importName(curSpecs[j])
		})

		deduped := curSpecs[:0]
		for i, s := range curSpecs {
			if i > 0 && collapse(curSpecs[i-1], s) {
				continue
			}

			deduped = append(deduped, s)
		}
		out[outIdx] = deduped
	}

	return out
}

func (f *formatter) importGroup(spec *dst.ImportSpec) int {
	specPath := importPath(spec)
	if f.localPrefix != "" && strings.HasPrefix(specPath, f.localPrefix) {
		return impGroupLocal
	}

	if idx := strings.Index(specPath, "/"); idx > 0 {
		specPath = specPath[:idx]
	}

	if strings.Contains(specPath, ".") {
		return impGroup3rdParty
	}

	return impGroupSTD
}

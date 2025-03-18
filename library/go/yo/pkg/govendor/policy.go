package govendor

import (
	"fmt"
	"path"
	"reflect"
	"strings"

	"a.yandex-team.ru/library/go/yo/pkg/yamake"
)

type policy struct {
	allow map[string]int
	deny  map[string]int

	denyTests map[string]struct{}
}

func (p *policy) allowsTests(importPath string) bool {
	for {
		if importPath == "." {
			return true
		}

		if _, deny := p.denyTests[importPath]; deny {
			return false
		}

		if _, deny := p.denyTests[importPath+"/"]; deny {
			return false
		}

		importPath = path.Dir(importPath)
	}
}

func (p *policy) allows(importPath string) bool {
	var minAllowIndex, minDenyIndex int

	checkPath := func(importPath string) {
		allowIndex, ok := p.allow[importPath]
		if ok {
			if minAllowIndex == 0 || allowIndex < minAllowIndex {
				minAllowIndex = allowIndex
			}
		}

		denyIndex, ok := p.deny[importPath]
		if ok {
			if minDenyIndex == 0 || denyIndex < minDenyIndex {
				minDenyIndex = denyIndex
			}
		}
	}

	for {
		if importPath == "." {
			if minAllowIndex == 0 {
				return false
			}

			if minDenyIndex == 0 {
				return true
			}

			return minAllowIndex < minDenyIndex
		}

		checkPath(importPath)
		importPath = path.Dir(importPath)
		checkPath(importPath + "/")
	}
}

const (
	vendorPrefix = "vendor/"
	testSuffix   = ";test"
)

var allowVendor = &yamake.PolicyDirective{
	Type:     yamake.PolicyDirectiveAllow,
	Package:  yamake.PolicyConsumerAny,
	Consumer: vendorPrefix,
}

var denyVendor = &yamake.PolicyDirective{
	Type:     yamake.PolicyDirectiveDeny,
	Package:  vendorPrefix,
	Consumer: yamake.PolicyConsumerAny,
}

func preparePolicy(list []*yamake.PolicyDirective) (*policy, error) {
	p := &policy{
		allow:     map[string]int{},
		deny:      map[string]int{},
		denyTests: map[string]struct{}{},
	}

	for i, directive := range list {
		if reflect.DeepEqual(directive, denyVendor) {
			continue
		}

		if reflect.DeepEqual(directive, allowVendor) {
			continue
		}

		if !strings.HasPrefix(directive.Package, vendorPrefix) {
			continue
		}

		importPath := strings.TrimPrefix(directive.Package, vendorPrefix)

		// Trim trailing .*. It has no effect on policy.
		importPath = strings.TrimSuffix(importPath, yamake.PolicyConsumerAny)

		// Try to detect regexps and complain.
		if strings.Contains(importPath, yamake.PolicyConsumerAny) {
			return nil, fmt.Errorf("policy regexp is not supported: %s", directive)
		}

		switch directive.Type {
		case yamake.PolicyDirectiveAllow:
			p.allow[importPath] = i

		case yamake.PolicyDirectiveDeny:
			if strings.HasSuffix(importPath, testSuffix) {
				importPath = strings.TrimSuffix(importPath, testSuffix)
				p.denyTests[importPath] = struct{}{}
			} else {
				p.deny[importPath] = i
			}
		}
	}

	return p, nil
}

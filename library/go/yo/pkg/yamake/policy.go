package yamake

import (
	"bufio"
	"fmt"
	"io"
	"os"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	PolicyDirectiveAllow = "ALLOW"
	PolicyDirectiveDeny  = "DENY"

	PolicyConsumerAny = ".*"

	policyPartsCount = 4
)

type PolicyDirective struct {
	Type     string // ALLOW / DENY
	Consumer string // .* or explicit Arcadia project
	Package  string // target Arcadia path
}

func (p PolicyDirective) String() string {
	return fmt.Sprintf("%s %s -> %s", p.Type, p.Consumer, p.Package)
}

// ReadPolicy parses .policy file and returns directives
func ReadPolicy(path string, directiveTypes ...string) ([]*PolicyDirective, error) {
	fd, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer fd.Close()

	return ParsePolicy(fd, directiveTypes...)
}

func ParsePolicy(r io.Reader, directiveTypes ...string) ([]*PolicyDirective, error) {
	if len(directiveTypes) == 0 {
		directiveTypes = []string{PolicyDirectiveAllow, PolicyDirectiveDeny}
	}

	var directives []*PolicyDirective

	sc := bufio.NewScanner(r)
	for sc.Scan() {
		line := strings.TrimSpace(sc.Text())

		suitableType := false
		for _, dt := range directiveTypes {
			if strings.HasPrefix(line, dt) {
				suitableType = true
				break
			}
		}
		if !suitableType {
			continue
		}

		directive, err := ParsePolicyDirective(line)
		if err != nil {
			return nil, xerrors.Errorf("cannot parse policy %q: %w", line, err)
		}

		directives = append(directives, directive)
	}

	if err := sc.Err(); err != nil {
		return nil, err
	}

	return directives, nil
}

func ParsePolicyDirective(directive string) (*PolicyDirective, error) {
	parts := strings.Fields(directive)
	if len(parts) != policyPartsCount {
		return nil, fmt.Errorf("invalid directive: %s", directive)
	}

	if parts[0] != PolicyDirectiveAllow &&
		parts[0] != PolicyDirectiveDeny {
		return nil, fmt.Errorf("bad directive start: %s", parts[0])
	}

	if parts[2] != "->" {
		return nil, fmt.Errorf("bad directive delimiter: %s", parts[2])
	}

	return &PolicyDirective{
		Type:     parts[0],
		Consumer: parts[1],
		Package:  parts[3],
	}, nil
}

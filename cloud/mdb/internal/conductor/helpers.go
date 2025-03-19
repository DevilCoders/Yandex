package conductor

import (
	"context"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	ErrEmptyGroup = xerrors.NewSentinel("empty conductor group name")
	ErrEmptyDC    = xerrors.NewSentinel("empty datacenter name")
)

func ParseGroup(arg string) (ParsedGroup, bool, error) {
	if arg == "" {
		return ParsedGroup{}, false, nil
	}

	if arg[0] != '%' {
		return ParsedGroup{}, false, nil
	}

	// Remove '%'
	arg = arg[1:]
	if arg == "" {
		return ParsedGroup{}, false, ErrEmptyGroup
	}

	// Do we have datacenter specified?
	idx := strings.LastIndex(arg, "@")
	if idx == -1 {
		return ParsedGroup{Name: arg}, true, nil
	}

	// Recheck for empty group name
	if idx == 0 {
		return ParsedGroup{}, false, ErrEmptyGroup
	}

	if len(arg) == idx+1 {
		return ParsedGroup{}, false, ErrEmptyDC
	}

	return ParsedGroup{Name: arg[:idx], DC: optional.NewString(arg[idx+1:])}, true, nil
}

type ParsedGroup struct {
	Name string
	DC   optional.String
}

func (pg ParsedGroup) String() string {
	if pg.DC.Valid {
		return fmt.Sprintf("%s@%s", pg.Name, pg.DC.String)
	}

	return pg.Name
}

type ParsedTarget struct {
	Hosts    []string
	Negative bool
}

var (
	ErrEmptyTarget = xerrors.NewSentinel("empty target")
)

func ParseTarget(ctx context.Context, client Client, arg string) (ParsedTarget, error) {
	if arg == "" {
		return ParsedTarget{}, ErrEmptyTarget
	}

	var pt ParsedTarget
	if arg[0] == '-' {
		pt.Negative = true
		arg = arg[1:]
	}

	group, ok, err := ParseGroup(arg)
	if err != nil {
		return ParsedTarget{}, xerrors.Errorf("error parsing conductor group %q: %w", arg, err)
	}

	// Is it a conductor group?
	if !ok {
		pt.Hosts = []string{arg}
		return pt, nil
	}

	pt.Hosts, err = client.GroupToHosts(ctx, group.Name, GroupToHostsAttrs{DC: group.DC})
	if err != nil {
		return ParsedTarget{}, xerrors.Errorf("error resolving conductor group %q to hosts: %w", group, err)
	}

	return pt, nil
}

func ParseMultiTarget(ctx context.Context, client Client, arg string) ([]ParsedTarget, error) {
	var targets []ParsedTarget
	multi := strings.Split(arg, ",")
	for _, single := range multi {
		target, err := ParseTarget(ctx, client, single)
		if err != nil {
			return nil, err
		}

		targets = append(targets, target)
	}

	return targets, nil
}

func ParseMultiTargets(ctx context.Context, client Client, args ...string) ([]ParsedTarget, error) {
	var targets []ParsedTarget
	for _, arg := range args {
		pts, err := ParseMultiTarget(ctx, client, arg)
		if err != nil {
			return nil, err
		}

		targets = append(targets, pts...)
	}

	return targets, nil
}

func CollapseTargets(targets []ParsedTarget) []string {
	unique := make(map[string]struct{})

	// Add positive targets
	for _, target := range targets {
		if target.Negative {
			continue
		}

		for _, host := range target.Hosts {
			unique[host] = struct{}{}
		}
	}

	// Remove negative targets
	for _, target := range targets {
		if !target.Negative {
			continue
		}

		for _, host := range target.Hosts {
			delete(unique, host)
		}
	}

	var res []string
	for k := range unique {
		res = append(res, k)
	}

	return res
}

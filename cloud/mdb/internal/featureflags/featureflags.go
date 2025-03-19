package featureflags

import "a.yandex-team.ru/cloud/mdb/internal/semerr"

type FeatureFlags struct {
	m map[string]struct{}
	l []string
}

func NewFeatureFlags(flags []string) FeatureFlags {
	var collision bool
	ffs := FeatureFlags{m: make(map[string]struct{})}
	for _, flag := range flags {
		if _, ok := ffs.m[flag]; ok {
			collision = true
		}

		ffs.m[flag] = struct{}{}
	}

	if collision {
		ffs.l = make([]string, 0, len(ffs.m))
		for flag := range ffs.m {
			ffs.l = append(ffs.l, flag)
		}
	} else {
		ffs.l = flags
	}

	return ffs
}

func (ffs *FeatureFlags) Has(flag string) bool {
	_, ok := ffs.m[flag]
	return ok
}

func (ffs *FeatureFlags) EnsurePresent(flag string) error {
	if ffs.Has(flag) {
		return nil
	}
	return semerr.Authorization("requested feature is not available")
}

func (ffs *FeatureFlags) List() []string {
	// nil slice interpreted in pgtype.TextArray as NULL, but we need empty array, same in worker task args.
	if ffs.l == nil {
		return []string{}
	}

	return ffs.l
}

package prometheus

import (
	"a.yandex-team.ru/library/go/core/metrics/internal/pkg/registryutil"
)

type RegistryOpts struct {
	Prefix string
	Tags   map[string]string
}

// NewRegistryOpts returns new initialized instance of RegistryOpts
func NewRegistryOpts() *RegistryOpts {
	return &RegistryOpts{
		Tags: make(map[string]string),
	}
}

// SetTags overrides existing tags
func (o *RegistryOpts) SetTags(tags map[string]string) *RegistryOpts {
	o.Tags = tags
	return o
}

// AddTags merges given tags with existing
func (o *RegistryOpts) AddTags(tags map[string]string) *RegistryOpts {
	for k, v := range tags {
		o.Tags[k] = v
	}
	return o
}

// SetPrefix overrides existing prefix
func (o *RegistryOpts) SetPrefix(prefix string) *RegistryOpts {
	o.Prefix = prefix
	return o
}

// AppendPrefix adds given prefix as postfix to existing using separator
func (o *RegistryOpts) AppendPrefix(prefix string) *RegistryOpts {
	o.Prefix = registryutil.BuildFQName("_", o.Prefix, prefix)
	return o
}

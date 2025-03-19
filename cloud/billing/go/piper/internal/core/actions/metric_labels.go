package actions

import (
	"sort"
	"strconv"
	"strings"

	"github.com/OneOfOne/xxhash"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
)

func labelsJSONHash(l entities.Labels) (r entities.HashedLabel) {
	// TODO: after reimplementing with system labels
	// if len(l.System) == 0 && len(l.User) == 0 {
	// 	return
	// }
	if len(l.User) == 0 {
		return
	}
	b := strings.Builder{}

	// _, _ = b.WriteString(`{"system_labels": {`)
	// dumpJSONLabelsMap(&b, l.System)

	_, _ = b.WriteString(`}, "user_labels": {`)
	dumpJSONLabelsMap(&b, l.User)

	_, _ = b.WriteString(`}}`)

	r.LabelsData = b.String()
	r.LabelsHash = xxhash.ChecksumString64(r.LabelsData)
	return
}

func dumpJSONLabelsMap(b *strings.Builder, m map[string]string) {
	if len(m) == 0 {
		return
	}

	keys := make(sort.StringSlice, 0, len(m))
	for k := range m {
		keys = append(keys, k)
	}
	keys.Sort()

	for i, k := range keys {
		if i > 0 {
			_, _ = b.WriteString(", ")
		}
		_, _ = b.WriteString(strconv.QuoteToASCII(k))
		_, _ = b.WriteString(`: `)
		_, _ = b.WriteString(strconv.QuoteToASCII(m[k]))
	}
}

package kikimr

import (
	"bytes"
	"fmt"
	"strings"

	"go.uber.org/zap"
	"golang.org/x/net/context"

	log "a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/misc"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/storage"
)

func buildListFields(fields []string, prefix string, prefixMap map[string]string) string {
	var q bytes.Buffer
	for i, field := range fields {
		if i > 0 {
			q.WriteString(", ")
		}
		if v, ok := prefixMap[field]; ok {
			q.WriteString(v)
		} else {
			q.WriteString(prefix)
		}
		q.WriteString(field)
	}
	return q.String()
}

func buildListSelector(r *storage.ListRequest, prefix string) (string, []interface{}) {
	var args []interface{}
	var clauses []string
	if r.Disk != "" {
		args = append(args, r.Disk)
		clauses = append(clauses, fmt.Sprintf("%vdisk = $%v", prefix, len(args)))
	}
	if r.Project != "" {
		args = append(args, r.Project)
		clauses = append(clauses, fmt.Sprintf("%vproject = $%v", prefix, len(args)))
	}
	if r.BillingStart != nil {
		args = append(args, kikimrTime{r.BillingStart})
		clauses = append(clauses, fmt.Sprintf("(%vdeleted IS NULL OR %vdeleted >= $%v)", prefix, prefix, len(args)))
	}
	if r.BillingEnd != nil {
		args = append(args, kikimrTime{r.BillingEnd})
		clauses = append(clauses, fmt.Sprintf("(%vcreated <= $%v)", prefix, len(args)))
	}
	if r.BillingStart == nil && r.BillingEnd == nil {
		args = append(args, storage.StateDeleting, storage.StateDeleted, storage.StateRogueChunks)
		clauses = append(clauses, fmt.Sprintf("%vstate NOT IN ($%v, $%v, $%v)", prefix, len(args)-2, len(args)-1, len(args)))
	} else {
		args = append(args, storage.StateCreating, storage.StateFailed)
		clauses = append(clauses, fmt.Sprintf("%vstate NOT IN ($%v, $%v)", prefix, len(args)-1, len(args)))
	}
	if r.Last != "" {
		args = append(args, r.Last)
		clauses = append(clauses, fmt.Sprintf("%vid > $%v", prefix, len(args)))
	}
	if r.SearchPrefix != "" {
		value := fmt.Sprintf("%v%%", r.SearchPrefix)
		args = append(args, value, value)
		// TODO: YQL-2921
		clauses = append(clauses, fmt.Sprintf("(%vdescription LIKE $%v OR %vname LIKE $%v)", prefix, len(args)-1, prefix, len(args)))
	}
	if r.SearchFull != "" {
		value := fmt.Sprintf("%%%v%%", r.SearchFull)
		args = append(args, value, value)
		// TODO: YQL-2921
		clauses = append(clauses, fmt.Sprintf("(%vdescription LIKE $%v OR %vname LIKE $%v)", prefix, len(args)-1, prefix, len(args)))
	}
	if r.CreatedAfter != nil {
		args = append(args, kikimrTime{r.CreatedAfter})
		clauses = append(clauses, fmt.Sprintf("(%vcreated >= $%v)", prefix, len(args)))
	}
	if r.CreatedBefore != nil {
		args = append(args, kikimrTime{r.CreatedBefore})
		clauses = append(clauses, fmt.Sprintf("(%vcreated < $%v)", prefix, len(args)))
	}

	return strings.Join(clauses, " AND "), args
}

type sortEntry struct {
	Field string
	Desc  bool
}

type sorting []sortEntry

func newSorting(ctx context.Context, s string) (sorting, error) {
	if s == "" {
		return nil, nil
	}

	// Format: _|[-]<field>(,[-]<field>)*
	sortFields := map[string]bool{
		"name":        false,
		"description": false,
		"created":     false,
		"realsize":    false,
	}

	params := strings.Split(s, ",")
	sort := make(sorting, 0, len(params))
	for _, param := range params {
		var desc bool
		if len(param) > 0 && param[0] == '-' {
			param = param[1:]
			desc = true
		}
		// Ignore case and underscores.
		param = strings.ToLower(strings.Replace(param, "_", "", -1))

		v, ok := sortFields[param]
		if !ok {
			log.G(ctx).Error("Sorting: invalid field", zap.String("param", param), zap.Error(misc.ErrSortingInvalidField))
			return nil, misc.ErrSortingInvalidField
		}
		if v {
			log.G(ctx).Error("Sorting: duplicate field", zap.String("param", param), zap.Error(misc.ErrSortingDuplicateField))
			return nil, misc.ErrSortingDuplicateField
		}
		sortFields[param] = true

		sort = append(sort, sortEntry{
			Field: param,
			Desc:  desc,
		})
	}

	return sort, nil
}

func (s sorting) OrderBy() string {
	if len(s) == 0 {
		return ""
	}

	var q bytes.Buffer
	q.WriteString(" ORDER BY ")
	for i, entry := range s {
		if i > 0 {
			q.WriteString(", ")
		}

		q.WriteString(entry.Field)
		q.WriteString(" ")
		if entry.Desc {
			q.WriteString("DESC")
		} else {
			q.WriteString("ASC")
		}
	}
	return q.String()
}

func (s sorting) OrderByID(prefix string) string {
	if len(s) > 0 {
		return s.OrderBy() + ", " + prefix + "id ASC "
	}
	return " ORDER BY " + prefix + "id ASC "
}

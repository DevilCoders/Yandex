package swag

import (
	"encoding/json"
	"fmt"
	"io"

	"github.com/go-openapi/loads"
	"github.com/go-openapi/spec"
)

func Mixin(primary *spec.Swagger, mixins ...*spec.Swagger) uint {
	var skipped uint
	if primary.Paths.Paths == nil {
		primary.Paths.Paths = map[string]spec.PathItem{}
	}
	opIds := getOpIds(primary)
	for i, m := range mixins {
		for k, v := range m.Definitions {
			// assume name collisions represent IDENTICAL type. careful.
			if _, exists := primary.Definitions[k]; exists {
				skipped++
				continue
			}
			primary.Definitions[k] = v
		}
		for k, v := range m.Paths.Paths {
			if _, exists := primary.Paths.Paths[k]; exists {
				skipped++
				continue
			}
			piops := pathItemOps(v)
			for _, piop := range piops {
				if opIds[piop.ID] {
					piop.ID = fmt.Sprintf("%v%v%v", piop.ID, "Mixin", i)
				}
				opIds[piop.ID] = true
			}
			primary.Paths.Paths[k] = v
		}
		for k, v := range m.Parameters {
			// could try to rename on conflict but would
			// have to fix $refs in the mixin. Complain
			// for now
			if _, exists := primary.Parameters[k]; exists {
				skipped++
				continue
			}
			primary.Parameters[k] = v
		}
		for k, v := range m.Responses {
			// could try to rename on conflict but would
			// have to fix $refs in the mixin. Complain
			// for now
			if _, exists := primary.Responses[k]; exists {
				skipped++
				continue
			}
			primary.Responses[k] = v
		}
	}
	return skipped
}

// MixinFiles is a convenience function for Mixin that reads the given
// swagger files, adds the mixins to primary, calls
// FixEmptyResponseDescriptions on the primary, and writes the primary
// with mixins to the given writer in JSON.  Returns the number of
// collsions that occured from mixins and any error.
func MixinFiles(primaryFile string, mixinFiles []string, w io.Writer) (uint, error) {
	primaryDoc, err := loads.Spec(primaryFile)
	if err != nil {
		return 0, err
	}
	primary := primaryDoc.Spec()

	var mixins []*spec.Swagger
	for _, mixinFile := range mixinFiles {
		mixin, err := loads.Spec(mixinFile)
		if err != nil {
			return 0, err
		}
		mixins = append(mixins, mixin.Spec())
	}

	collisions := Mixin(primary, mixins...)
	FixEmptyResponseDescriptions(primary)

	bs, err := json.MarshalIndent(primary, "", "  ")
	if err != nil {
		return 0, err
	}

	_, _ = w.Write(bs)

	return collisions, nil
}

// FixEmptyResponseDescriptions replaces empty ("") response
// descriptions in the input with "(empty)" to ensure that the
// resulting Swagger is stays valid.  The problem appears to arise
// from reading in valid specs that have a explicit response
// description of "" (valid, response.description is required), but
// due to zero values being omitted upon re-serializing (omitempty) we
// lose them unless we stick some chars in there.
func FixEmptyResponseDescriptions(s *spec.Swagger) {
	for _, v := range s.Paths.Paths {
		if v.Get != nil {
			FixEmptyDescs(v.Get.Responses)
		}
		if v.Put != nil {
			FixEmptyDescs(v.Put.Responses)
		}
		if v.Post != nil {
			FixEmptyDescs(v.Post.Responses)
		}
		if v.Delete != nil {
			FixEmptyDescs(v.Delete.Responses)
		}
		if v.Options != nil {
			FixEmptyDescs(v.Options.Responses)
		}
		if v.Head != nil {
			FixEmptyDescs(v.Head.Responses)
		}
		if v.Patch != nil {
			FixEmptyDescs(v.Patch.Responses)
		}
	}
	for k, v := range s.Responses {
		FixEmptyDesc(&v)
		s.Responses[k] = v
	}
}

// FixEmptyDescs adds "(empty)" as the description for any Response in
// the given Responses object that doesn't already have one.
func FixEmptyDescs(rs *spec.Responses) {
	FixEmptyDesc(rs.Default)
	for k, v := range rs.StatusCodeResponses {
		FixEmptyDesc(&v)
		rs.StatusCodeResponses[k] = v
	}
}

// FixEmptyDesc adds "(empty)" as the description to the given
// Response object if it doesn't already have one and isn't a
// ref. No-op on nil input.
func FixEmptyDesc(rs *spec.Response) {
	if rs == nil || rs.Description != "" || rs.Ref.Ref.GetURL() != nil {
		return
	}
	rs.Description = "(empty)"
}

// getOpIds extracts all the paths.<path>.operationIds from the given
// spec and returns them as the keys in a map with 'true' values.
func getOpIds(s *spec.Swagger) map[string]bool {
	rv := make(map[string]bool)
	for _, v := range s.Paths.Paths {
		piops := pathItemOps(v)
		for _, op := range piops {
			rv[op.ID] = true
		}
	}
	return rv
}

func pathItemOps(p spec.PathItem) []*spec.Operation {
	var rv []*spec.Operation
	rv = appendOp(rv, p.Get)
	rv = appendOp(rv, p.Put)
	rv = appendOp(rv, p.Post)
	rv = appendOp(rv, p.Delete)
	rv = appendOp(rv, p.Head)
	rv = appendOp(rv, p.Patch)
	return rv
}

func appendOp(ops []*spec.Operation, op *spec.Operation) []*spec.Operation {
	if op == nil {
		return ops
	}
	return append(ops, op)
}

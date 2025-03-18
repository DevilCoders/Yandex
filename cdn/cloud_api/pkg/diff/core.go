package diff

import (
	"encoding/json"

	"github.com/pmezard/go-difflib/difflib"
)

const jsonIndent = "    "

func CalculateDiff(name1 string, entity1 interface{}, name2 string, entity2 interface{}) (string, error) {
	entity1JSON, err := json.MarshalIndent(entity1, "", jsonIndent)
	if err != nil {
		return "", err
	}

	entity2JSON, err := json.MarshalIndent(entity2, "", jsonIndent)
	if err != nil {
		return "", err
	}

	entity1Lines := difflib.SplitLines(string(entity1JSON))
	entity2Lines := difflib.SplitLines(string(entity2JSON))

	diff := difflib.UnifiedDiff{
		A:        entity1Lines,
		FromFile: name1,
		B:        entity2Lines,
		ToFile:   name2,
		Context:  max(len(entity1Lines), len(entity2Lines)),
	}

	diffString, err := difflib.GetUnifiedDiffString(diff)
	if err != nil {
		return "", err
	}

	return diffString, nil
}

func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}

package saltapi

import (
	"encoding/json"
	"sort"
	"strings"
)

type ReturnStatesMap map[string]*StateReturn

// map's key matters:
type returnStatesPair = struct {
	key   string
	value *StateReturn
}

type byRunNum []returnStatesPair

func (a byRunNum) Len() int           { return len(a) }
func (a byRunNum) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a byRunNum) Less(i, j int) bool { return a[i].value.RunNum < a[j].value.RunNum }

func (c ReturnStatesMap) MarshalJSON() ([]byte, error) {
	// this code prints ReturnStates in RunNum order:
	pairs := make([]returnStatesPair, 0)
	for k, v := range c {
		pairs = append(pairs, returnStatesPair{
			key:   k,
			value: v,
		})
	}

	// sort by RunNum:
	sort.Sort(byRunNum(pairs))

	// Print all data:
	var result strings.Builder
	result.WriteString("{")

	var firstDone bool
	for _, pair := range pairs {
		if firstDone {
			result.WriteString(",")
		}

		data, err := json.Marshal(pair.key)
		if err != nil {
			return nil, err
		}
		result.WriteString(string(data))
		result.WriteString(": ")

		data, err = json.Marshal(pair.value)
		if err != nil {
			return nil, err
		}
		result.WriteString(string(data))

		firstDone = true
	}
	result.WriteString("}")

	return []byte(result.String()), nil
}

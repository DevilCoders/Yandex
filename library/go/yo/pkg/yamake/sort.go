package yamake

import (
	"sort"
	"strings"
)

func SortRecurse(yaMake *YaMake) {
	items, comments := separateComments(yaMake.Recurse)
	pairs := enumerate(items)
	sort.Slice(pairs, func(i, j int) bool {
		return pairs[i].item < pairs[j].item
	})
	perm, items := pairs.Split()
	comments = comments.ApplyPermutation(perm.Inverse())
	yaMake.Recurse = mergeComments(items, comments)
}

func separateComments(itemsWithComments []string) (items []string, comments Comments) {
	// we might allocate more than we need, but it's ok
	items = make([]string, 0, len(itemsWithComments))
	comments = make(map[int][]string)
	for _, val := range itemsWithComments {
		if isComment(val) {
			// index is the index of the corresponding item for the comment.
			// We assume that this is the item which goes after the comment.
			// If there are no items after the comment, then we handle the special case.
			index := len(items)
			comments[index] = append(comments[index], val)
			continue
		}
		items = append(items, val)
	}
	return
}

func mergeComments(items []string, comments map[int][]string) []string {
	// we might allocate less than we need, but it's ok
	result := make([]string, 0, len(items))
	for i, item := range items {
		itemComments := comments[i]
		result = append(result, itemComments...)
		result = append(result, item)
	}
	// handle case when some comments are after all the items
	result = append(result, comments[len(items)]...)
	return result
}

func isComment(item string) bool {
	return strings.HasPrefix(item, "#")
}

type Pair struct {
	index int
	item  string
}
type Permutation []int

// Inverse create an inverse permutation
func (p Permutation) Inverse() Permutation {
	result := make(Permutation, len(p))
	for i, j := range p {
		result[j] = i
	}
	return result
}

type Comments map[int][]string

func (c Comments) ApplyPermutation(perm Permutation) Comments {
	result := make(Comments)
	for i, coms := range c {
		if i < len(perm) {
			result[perm[i]] = coms
		} else {
			// handle case when there are comments after all the items
			result[i] = coms
		}
	}
	return result
}

type Pairs []Pair

func (p Pairs) Split() (Permutation, []string) {
	perm := make(Permutation, 0, len(p))
	items := make([]string, 0, len(p))
	for _, pair := range p {
		perm = append(perm, pair.index)
		items = append(items, pair.item)
	}
	return perm, items
}

func enumerate(items []string) Pairs {
	result := make([]Pair, 0, len(items))
	for i, item := range items {
		result = append(result, Pair{index: i, item: item})
	}
	return result
}

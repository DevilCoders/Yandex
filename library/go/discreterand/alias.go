// Original implementation by Damian Gryski <damian@gryski.com>
// at https://github.com/dgryski/go-discreterand under the MIT license.
// Package discreterand provides constant time sampling from a discrete distribution

/*
This is an implementation of Vose's alias method for
choosing elements from a discrete distribution.
For a full description of the algorithm, see http://www.keithschwarz.com/darts-dice-coins/
*/
package discreterand

import (
	"math/rand"
)

// AliasTable is a discrete distribution
type AliasTable struct {
	rnd   *rand.Rand
	alias []int
	prob  []float64
}

// slice-based stack
type worklist []int

func (w *worklist) push(i int) {
	*w = append(*w, i)
}

func (w *worklist) pop() int {
	l := len(*w) - 1
	n := (*w)[l]
	(*w) = (*w)[:l]
	return n
}

// NewAlias constructs an AliasTable  that will generate the discrete distribution given in probabilities.
// The probabilities slice should sum to 1.
func NewAlias(probabilities []float64, src rand.Source) AliasTable {
	n := len(probabilities)

	v := AliasTable{}

	v.alias = make([]int, n)
	v.prob = make([]float64, n)
	v.rnd = rand.New(src)

	p := make([]float64, n)
	for i := 0; i < n; i++ {
		p[i] = probabilities[i] * float64(n)
	}

	var small worklist
	var large worklist

	for i, pi := range p {
		if pi < 1 {
			small = append(small, i)
		} else {
			large = append(large, i)
		}
	}

	for len(large) > 0 && len(small) > 0 {
		l := small.pop()
		g := large.pop()
		v.prob[l] = p[l]
		v.alias[l] = g

		p[g] = (p[g] + p[l]) - 1
		if p[g] < 1 {
			small.push(g)
		} else {
			large.push(g)
		}
	}

	for len(large) > 0 {
		g := large.pop()
		v.prob[g] = 1
	}

	for len(small) > 0 {
		l := small.pop()
		v.prob[l] = 1
	}

	return v
}

// Next returns the next random value from the discrete distribution
func (v *AliasTable) Next() int {
	i := v.rnd.Intn(len(v.alias))

	if v.rnd.Float64() < v.prob[i] {
		return i
	}
	return v.alias[i]
}

package shift

import (
	"sort"
	"time"

	"go.mongodb.org/mongo-driver/bson/primitive"
)

// Shift represents something
type Shift struct {
	ID        primitive.ObjectID `bson:"_id,omitempty" json:"id,omitempty"`
	ServiceID string             `bson:"serviceid" json:"serviceid"`
	DateStart int64              `bson:"datestart" json:"datestart"`
	DateEnd   int64              `bson:"dateend" json:"dateend"`
	Resp      Resp               `bson:"resp" json:"resp"`
}

// Resp represents something
type Resp struct {
	Order    int    `bson:"order" json:"order"`
	Username string `bson:"username" json:"username"`
}

// FlattenShift returns list representation for users in shs
func FlattenShift(shs []Shift) (resps []string) {
	ds := make([]int64, 0)
	for _, sh := range shs {
		ds = append(ds, sh.DateStart)
	}

	sort.Slice(ds, func(i, j int) bool {
		return ds[i] < ds[j]
	})

	flat := make(map[int64][]Resp)
	for _, sh := range shs {
		flat[sh.DateStart] = append(flat[sh.DateStart], sh.Resp)
	}

	dedup := make(map[string]bool)
	for _, d := range ds {
		rs := flat[d]
		sort.Slice(rs, func(i, j int) bool {
			return rs[i].Order < rs[j].Order
		})
		for _, r := range rs {
			if !dedup[r.Username] {
				resps = append(resps, r.Username)
			}
			dedup[r.Username] = true
		}
	}
	return
}

// AddUserToFlattenResps puts username into given position
func AddUserToFlattenResps(resps []string, user string, pos int) (res []string) {
	if len(resps) <= pos {
		pos = len(resps)
	}

	if pos < 0 {
		pos = 0
	}

	for _, resp := range resps[:pos] {
		if resp == user {
			return resps
		}
	}

	for i, resp := range resps[pos:] {
		if resp == user {
			resps[i+pos], resps[pos] = resps[pos], resps[i+pos]
			return resps
		}
	}

	res = append(resps, "")
	copy(res[pos+1:], res[pos:])
	res[pos] = user
	return
}

// FilterToCurrent removes all shifts, which DateEnd is earlier than time.Now(). Sets DateStart of border shift to time.Now()
func FilterToCurrent(shs []Shift) (shr []Shift) {
	now := time.Now().Unix()
	for _, sh := range shs {
		if sh.DateEnd < now {
			continue
		}

		if sh.DateStart < now {
			sh.DateStart = now
		}

		shr = append(shr, sh)
	}

	return
}

// ApplyReplaces splits shift by inner shifts borders and replaces its Resp data accordingly
func ApplyReplaces(sh *Shift, srs []Shift) (shs []Shift) {
	var last *Shift // Replasement Shift from previous iteration
	for _, sr := range srs {
		cur := sr
		if last == nil {
			if sh.DateStart != sr.DateStart {
				shs = append(shs,
					Shift{
						ServiceID: sh.ServiceID,
						DateStart: sh.DateStart,
						DateEnd:   sr.DateStart,
						Resp:      sh.Resp,
					},
				)
			}
			last = &cur
			continue
		}

		// Case in which previous replacement shift doesn't interlapse with current one
		if last.DateEnd <= sr.DateStart {
			shs = append(shs,
				Shift{ // Add shift with borders and user of last and order of sh
					ServiceID: sh.ServiceID,
					DateStart: last.DateStart,
					DateEnd:   last.DateEnd,
					Resp: Resp{
						sh.Resp.Order,
						last.Resp.Username,
					},
				},
				Shift{ // Add shift between the last and sr
					ServiceID: sh.ServiceID,
					DateStart: last.DateEnd,
					DateEnd:   sr.DateStart,
					Resp:      sh.Resp,
				},
			)
		} else { // Interlapses
			shs = append(shs,
				Shift{
					ServiceID: sh.ServiceID,
					DateStart: last.DateStart,
					DateEnd:   sr.DateStart,
					Resp: Resp{
						sh.Resp.Order,
						last.Resp.Username,
					},
				},
			)
		}

		last = &cur
	}

	// After the main loop - add shift with borders and user of last and order of sh, if any replacements done
	if last != nil {
		shs = append(shs,
			Shift{
				ServiceID: sh.ServiceID,
				DateStart: last.DateStart,
				DateEnd:   last.DateEnd,
				Resp: Resp{
					sh.Resp.Order,
					last.Resp.Username,
				},
			},
		)
		if last.DateEnd != sh.DateEnd {
			shs = append(shs,
				Shift{
					ServiceID: sh.ServiceID,
					DateStart: last.DateEnd,
					DateEnd:   sh.DateEnd,
					Resp:      sh.Resp,
				})
		}
	}

	return
}

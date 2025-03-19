package service

import (
	"errors"
	"strconv"
)

// type abcError struct {
//     field  int
//     prob string
// }

var (
	ErrInvalidScheduleTime = errors.New("'Method': Importing method not supported")
	// ErrNoOwnerPresentAA     = errors.New("'Order': No 'lead' role found")
)

type ABC struct {
	ServiceID   uint64                 `bson:"service_id,omitempty" json:"service_id,omitempty"` // VERIFY: get by id
	ServiceSlug string                 `json:"service_slug,omitempty"`                           // VERIFY: get by slug
	Schedule    map[string]ABCSchedule `json:"schedule,omitempty"`
}

type ABCSchedule struct {
	Active          *bool    `json:"active,omitempty"`           // VERIFY: active is ptr
	FetchUnapproved *bool    `json:"fetch_unapproved,omitempty"` // VERIFY: active is ptr
	Squash          *bool    `json:"squash,omitempty"`           // VERIFY: active is ptr
	ID              uint64   `json:"id,omitempty"`               // VERIFY: get by id
	Slug            string   `json:"slug,omitempty"`             // VERIFY: get by slug
	Time            string   `json:"time,omitempty"`             // VERIFY: 00-23:00-59
	Order           []string `json:"order,omitempty"`            // VERIFY: is in s.Schedule.Order
}

// Validate fixes most common problems and checks integrity of fields
func Validate(s *Service) error {
	// ServiceID
	// ServiceSlug
	for _, sch := range s.Schedule.ABC.Schedule {
		if sch.Active == nil {
			sch.Active = new(bool)
		}

		if sch.FetchUnapproved == nil {
			sch.FetchUnapproved = new(bool)
		}

		if sch.Squash == nil {
			sch.Squash = new(bool)
		}
		// ID
		// Slug

		if len(sch.Time) != 5 {
			return ErrInvalidScheduleTime
		}

		if hours, err := strconv.ParseInt(sch.Time[:2], 10, 0); err != nil {
			return err
		} else if hours < 0 || hours > 23 {
			return ErrInvalidScheduleTime
		}

		if minutes, err := strconv.ParseInt(sch.Time[3:], 10, 0); err != nil {
			return err
		} else if minutes < 0 || minutes > 59 {
			return ErrInvalidScheduleTime
		}

		if len(sch.Order) < 1 {
			return ErrInvalidScheduleTime
		}

		for _, ord := range sch.Order {
			if _, ok := s.Schedule.Order[ord]; !ok {
				return ErrInvalidScheduleTime
			}
		}
	}
	return nil
}

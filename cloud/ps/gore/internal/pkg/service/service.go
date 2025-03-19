package service

import (
	"reflect"
	"strings"
	"time"

	"errors"

	"go.mongodb.org/mongo-driver/bson/primitive"
)

var (
	ErrNilPointer = errors.New("'Method': Importing method not supported")
	ErrA          = errors.New("'Order': No 'lead' role found")
)

// Service represents something
type Service struct {
	ID          primitive.ObjectID `bson:"_id,omitempty" json:"id,omitempty"`
	Service     string             `json:"service,omitempty"`     //slug сервиса, строка [a-z0-9-_]
	Description string             `json:"description,omitempty"` //нечто человекочитаемое, одной строкой
	Active      *bool              `json:"active,omitempty"`      //Общие положения, 5
	Timezone    string             `json:"timezone,omitempty"`    //В какой TZ расположен заказчик
	LastUpdated *time.Time         `bson:"lastupdated,omitempty" json:"lastupdated,omitempty"`
	TeamOwners  map[string]string  `json:"teamowners,omitempty"`
	Schedule    *Schedule          `json:"schedule,omitempty"`
	Calendar    *Calendar          `json:"calendar,omitempty"`
	Juggler     *Juggler           `json:"juggler,omitempty"`
	Golem       *Golem             `json:"golem,omitempty"`
	Startrack   *Startrack         `json:"startrack,omitempty"`
	Walle       []Walle            `json:"walle,omitempty"`
	IDM         *IDM               `json:"idm,omitempty"`
}

const (
	slugRE  = `[a-z][0-9a-z_-]{0,30}[a-z]`
	descLen = 255
)

var importAvailable = []string{
	"file:bitbucket",
	"file:github",
	"file:arcadia",
	"calendar",
	"abc",
	"round_robin",
}

var jsonFields []string

func init() {
	m := make(map[string]ABCSchedule)
	m["_"] = ABCSchedule{
		Active:          new(bool),
		FetchUnapproved: new(bool),
		Squash:          new(bool),
	}
	jsonFields = structTagLookup(&Service{
		Active:      new(bool),
		LastUpdated: &time.Time{},
		Schedule: &Schedule{
			Active: new(bool),
			ABC: &ABC{
				Schedule: m,
			},
			File: &File{
				HasHeader: new(bool),
			},
			RoundRobin: &RoundRobin{
				PeriodDays: 7,
			},
		},
		Calendar: &Calendar{
			Active: new(bool),
		},
		Juggler: &Juggler{
			Active:      new(bool),
			IncludeRest: new(bool),
		},
		Golem: &Golem{
			Active: new(bool),
		},
		Startrack: &Startrack{
			Active: new(bool),
			Template: &StTemplate{
				Active: new(bool),
			},
			Duty: &StDuty{
				Active:     new(bool),
				Continious: new(bool),
				Creation: &StCreation{
					Managed: new(bool),
				},
			},
		},
		Walle: []Walle{},
		IDM: &IDM{
			Active: new(bool),
		},
	}, "", ".")
}

func structTagLookup(v interface{}, parent, sep string) (ts []string) {
	t := reflect.ValueOf(v).Elem()
	typeOfT := t.Type()
	switch t.Kind() {
	default:
	case reflect.Struct:
		for i := 0; i < t.NumField(); i++ {
			valueField := t.Field(i)
			typeField := typeOfT.Field(i)
			if tag, ok := typeField.Tag.Lookup("json"); ok {
				tn := strings.Split(tag, ",")[0]
				switch typeField.Type.Kind() {
				case reflect.Map:
				case reflect.Ptr:
					for _, tt := range structTagLookup(valueField.Interface(), tn, sep) {
						ts = append(ts, tn+sep+tt)
					}
				case reflect.Struct:
					for _, tt := range structTagLookup(valueField.Addr().Interface(), tn, sep) {
						ts = append(ts, tn+sep+tt)
					}
				}
				ts = append(ts, tn)
			}
		}
	}

	return
}

func JSONFields() []string {
	return jsonFields
}

func ImportAvailable() []string {
	return importAvailable
}

func ImportAdd(imp string) {
	importAvailable = append(importAvailable, imp)
}

package template

import (
	"bytes"
	"fmt"
	"text/template"
	"time"

	"go.mongodb.org/mongo-driver/bson/primitive"
)

// Template represents common template struct
type Template struct {
	Active    bool               `json:"active"`
	ID        primitive.ObjectID `bson:"_id,omitempty" json:"id,omitempty"`
	ServiceID string             `json:"serviceid,omitempty"`
	Type      string             `json:"type,omitempty"`
	Text      string             `json:"text,omitempty"`
	Format    string             `json:"format,omitempty"`
	Unique    bool               `json:"-"`
}

// Variables used in rendering. Acessible via .Var notation
type Variables struct {
	User    string
	Service string
	Env     string
	Format  string
	Start   time.Time
	End     time.Time
}

// That's ugly, but I have no better ideas
var registredTemplates = map[string]Template{}

// RegisterTemplate adds new type and its default representation
func RegisterTemplate(s string, t Template) {
	if _, ok := registredTemplates[s]; ok {
		panic("Template conflict")
	}

	registredTemplates[s] = t
}

// DefaultTemplate returns registred template defaults
func DefaultTemplate(s string) (t Template, err error) {
	if t, ok := registredTemplates[s]; ok {
		return t, err
	}

	return t, fmt.Errorf("unregistered template type")
}

// Render returns compiled template string
func (t *Template) Render(v *Variables) (rend string, err error) {
	tpl, err := template.New("rend").Parse(t.Text)
	if err != nil {
		return
	}

	tps := new(bytes.Buffer)
	v.Format = t.Format
	if err = tpl.Execute(tps, v); err != nil {
		return
	}

	rend = tps.String()
	return
}

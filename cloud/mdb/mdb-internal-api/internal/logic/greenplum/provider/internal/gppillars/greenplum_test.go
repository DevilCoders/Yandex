package gppillars

import "testing"

func TestUnmarshalAllowUnknownFields(t *testing.T) {
	json := `{"data":{}, "bla":"bla"}`
	pillar := NewCluster()

	err := pillar.UnmarshalPillar([]byte(json))
	if err != nil {
		t.Fatal(err)
	}
}

func TestUnmarshalDisAllowUnknownFields(t *testing.T) {
	json := `{"data":{}, "bla":"bla"}`
	pillar := NewCluster()
	pillar.DisallowUnknownFields()
	err := pillar.UnmarshalPillar([]byte(json))
	if err == nil {
		t.Fatal(`Should be json: unknown field "bla"`)
	}
}

package provider

import (
	"testing"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/greenplum/provider/internal/gppillars"
)

func Test_generateSegments_low_group_size_odd_2(t *testing.T) {
	segments := map[int]gppillars.SegmentData{}
	hosts := []string{"1", "2", "3", "4", "5", "6", "7", "8", "9"}

	_, _, err := generateSegments(segments, 10, hosts, 1, true, 2)
	if err != nil {
		t.Error(err)
	}
}

func Test_generateSegments_low_group_size_odd_2_md(t *testing.T) {
	segments := map[int]gppillars.SegmentData{}
	hosts := []string{"1", "2", "3", "4", "5", "6", "7", "8", "9"}

	_, _, err := generateSegments(segments, 10, hosts, 1, false, 2)
	if err != nil {
		t.Error(err)
	}
}

func Test_generateSegments_low_group_size_even(t *testing.T) {
	segments := map[int]gppillars.SegmentData{}
	hosts := []string{"1", "2", "3", "4", "5", "6", "7", "8"}

	_, _, err := generateSegments(segments, 10, hosts, 1, true, 2)
	if err != nil {
		t.Error(err)
	}
}

func Test_generateSegments(t *testing.T) {
	segments := map[int]gppillars.SegmentData{}
	hosts := []string{"1", "2", "3", "4", "5", "6", "7", "8", "9"}

	_, _, err := generateSegments(segments, 10, hosts, 1, false, 4)
	if err != nil {
		t.Error(err)
	}

	if len(segments) != 9 {
		t.Errorf("generateSegments: len(segments) = %v should be 9", len(segments))
	}
}

func Test_generateSegments_2_seg_in_host(t *testing.T) {
	segments := map[int]gppillars.SegmentData{}
	hosts := []string{"1", "2", "3", "4", "5", "6", "7", "8", "9"}

	_, _, err := generateSegments(segments, 10, hosts, 2, true, 4)
	if err != nil {
		t.Error(err)
	}

	if len(segments) != 18 {
		t.Errorf("generateSegments: len(segments) = %v should be 18", len(segments))
	}

	// 1 4 7
	// 2 5 8
	// 3 6 9

	if segments[27].Primary.Fqdn != "9" {
		t.Errorf("generateSegments: segments[27].Primary.Fqdn = %v should be 9", segments[27].Primary.Fqdn)
	}

	if segments[27].Mirror.Fqdn != "6" {
		t.Errorf("generateSegments: segments[27].Mirror.Fqdn = %v should be 6", segments[27].Mirror.Fqdn)
	}

}

package tags

import (
	"fmt"

	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/internal/cloud/tags"
)

const (
	discountTagName = "map-migrated"
)

func DefaultDataPlaneTags(controlPlane, controlPlaneVersionID, projectID, discount string) []*ec2.Tag {
	return []*ec2.Tag{
		TeamMDB(),
		PlaneData(),
		ControlPlane(controlPlane),
		ControlPlaneVersionID(controlPlaneVersionID),
		ProjectID(projectID),
		DiscountTag(discount),
		AtDoubleCloud(),
	}
}

func Name(name string) *ec2.Tag {
	return newTag(tags.Name, name)
}

func Namef(format string, args ...interface{}) *ec2.Tag {
	return newTag(tags.Name, fmt.Sprintf(format, args...))
}

func Team(team string) *ec2.Tag {
	return newTag(tags.Team, team)
}

func TeamMDB() *ec2.Tag {
	return Team("MDB")
}

func Plane(plane string) *ec2.Tag {
	return newTag(tags.Plane, plane)
}

func PlaneData() *ec2.Tag {
	return Plane("Data")
}

func ControlPlane(cp string) *ec2.Tag {
	return newTag(tags.ControlPlane, cp)
}

func ControlPlaneVersionID(id string) *ec2.Tag {
	return newTag(tags.ControlPlaneVersionID, id)
}

func ProjectID(id string) *ec2.Tag {
	return newTag(tags.ProjectID, id)
}

func ClusterID(id string) *ec2.Tag {
	return newTag(tags.ClusterID, id)
}

func NetworkID(id string) *ec2.Tag {
	return newTag(tags.NetworkID, id)
}

func NetworkName(name string) *ec2.Tag {
	return newTag(tags.NetworkName, name)
}

func DiscountTag(value string) *ec2.Tag {
	return newTag(discountTagName, value)
}

func NetworkConnectionID(id string) *ec2.Tag {
	return newTag(tags.NetworkConnectionID, id)
}

func AtDoubleCloud() *ec2.Tag {
	return newTag("AtDoubleCloud", "true")
}

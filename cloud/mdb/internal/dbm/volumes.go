package dbm

type ContainerVolumes struct {
	FQDN    string
	Volumes []Volume
}

func (cv *ContainerVolumes) Total() int {
	result := 0
	for _, v := range cv.Volumes {
		result += v.SpaceLimit
	}
	return result
}

type Volume struct {
	SpaceLimit int
}

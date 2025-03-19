package clusters

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestModels_ZoneHostsList(t *testing.T) {
	t.Run("ZoneHostsListsToMap", func(t *testing.T) {
		list := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  1,
			},
			ZoneHosts{
				ZoneID: "eu-central-1b",
				Count:  1,
			},
			ZoneHosts{
				ZoneID: "eu-central-1c",
				Count:  1,
			},
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  1,
			},
		}

		res := list.Add(ZoneHostsList{})
		require.Equal(t,
			ZoneHostsList{
				ZoneHosts{
					ZoneID: "eu-central-1a",
					Count:  2,
				},
				ZoneHosts{
					ZoneID: "eu-central-1b",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1c",
					Count:  1,
				},
			},
			res,
		)
	})

	baseList := ZoneHostsList{
		ZoneHosts{
			ZoneID: "eu-central-1b",
			Count:  1,
		},
		ZoneHosts{
			ZoneID: "eu-central-1a",
			Count:  1,
		},
		ZoneHosts{
			ZoneID: "eu-central-1c",
			Count:  1,
		},
	}

	t.Run("AddPresent", func(t *testing.T) {
		addList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  1,
			},
			ZoneHosts{
				ZoneID: "eu-central-1c",
				Count:  1,
			},
		}

		res := baseList.Add(addList)
		require.Equal(t,
			ZoneHostsList{
				ZoneHosts{
					ZoneID: "eu-central-1a",
					Count:  2,
				},
				ZoneHosts{
					ZoneID: "eu-central-1b",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1c",
					Count:  2,
				},
			},
			res,
		)
	})

	t.Run("AddNotPresented", func(t *testing.T) {
		addList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1d",
				Count:  1,
			},
		}

		res := baseList.Add(addList)
		require.Equal(t,
			ZoneHostsList{
				ZoneHosts{
					ZoneID: "eu-central-1a",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1b",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1c",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1d",
					Count:  1,
				},
			},
			res,
		)
	})

	t.Run("Sub", func(t *testing.T) {
		subList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1b",
				Count:  1,
			},
		}

		res, err := baseList.Sub(subList)
		require.NoError(t, err)
		require.Equal(t,
			ZoneHostsList{
				ZoneHosts{
					ZoneID: "eu-central-1a",
					Count:  1,
				},
				ZoneHosts{
					ZoneID: "eu-central-1c",
					Count:  1,
				},
			},
			res,
		)
	})

	t.Run("SubAfterAdd", func(t *testing.T) {
		addList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  2,
			},
			ZoneHosts{
				ZoneID: "eu-central-1b",
				Count:  5,
			},
			ZoneHosts{
				ZoneID: "eu-central-1c",
				Count:  3,
			},
		}

		subList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  3,
			},
			ZoneHosts{
				ZoneID: "eu-central-1b",
				Count:  1,
			},
			ZoneHosts{
				ZoneID: "eu-central-1c",
				Count:  4,
			},
		}

		var err error
		res := baseList.Add(addList)
		res, err = res.Sub(subList)
		require.NoError(t, err)
		require.Equal(t,
			ZoneHostsList{
				ZoneHosts{
					ZoneID: "eu-central-1b",
					Count:  5,
				},
			},
			res,
		)
	})

	t.Run("SubMissingKey", func(t *testing.T) {
		subList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1d",
				Count:  1,
			},
		}

		_, err := baseList.Sub(subList)
		require.Error(t, err)
	})

	t.Run("SubMoreThanExist", func(t *testing.T) {
		subList := ZoneHostsList{
			ZoneHosts{
				ZoneID: "eu-central-1a",
				Count:  3,
			},
		}

		_, err := baseList.Sub(subList)
		require.Error(t, err)
	})
}

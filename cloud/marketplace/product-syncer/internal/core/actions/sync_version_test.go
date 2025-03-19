package actions

import (
	"testing"

	"github.com/stretchr/testify/suite"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/marketplace/product-syncer/internal/core/model"
)

func TestVersionSyncRequest(t *testing.T) {
	suite.Run(t, new(VersionSyncRequestTestSuite))
}

type VersionSyncRequestTestSuite struct {
	suite.Suite
}

func (suite *VersionSyncRequestTestSuite) TestEmptyNetworkInterfaces() {
	input := []byte(`category_names:
- navigation-infrastructure-network
- network
created_at: 1653907478
id: eqg22igp4en6a2k299g8
license_rules: []
marketing_info:
  logo_source:
    bucket_name: products
    link: https://storage.yandexcloud.net/products/f2etqeet87jshce7o7j8.svg
    object_name: f2etqeet87jshce7o7j8.svg
  translations:
    en:
      description: 'test'
      links:
      - title: testtitle
        url: testurl
      name: NAT instance
      short_description: test
      support: test
      tutorial: ''
      use_cases: 'test'
      ya_support: test
    ru:
      description: тест
      links:
      - title: тест
        url: тест
      name: NAT-инстанс
      short_description: тест
      support: тест
      tutorial: ''
      use_cases: 'тест'
      ya_support: тест
payload:
  compute_image:
    family: nat-instance-ubuntu
    form_id: linux
    image_id: fd8pm25269c5gqs23eb9
    package_info:
      os:
        family: linux
        name: Ubuntu
        version: '18.04'
      package_contents:
      - name: Ubuntu
        version: '18.04'
    pool_size: 1
    resource_spec:
      compute_platforms:
      - standard-v1
      - standard-v2
      - standard-v3
      cpu:
        min: 2
      cpu_fraction:
        max: 100
        min: 5
      disk_size:
        min: 3221225472
      memory:
        min: 536870912
      network_interfaces:
        max: 8
        min: 1
    valid_base_image: true
pricing:
  skus: []
  tariff_id: f2ejadoqeg5f3lu79fo7
  type: free
product_id: eqgtqeet87jshce7o7j8
publisher_id: eqgm8lfjgq7u9jo8nv8p
readonly: false
related_products: []
restrictions: {}
source:
  compute_image:
    build_id: f2ekjfoj1o2or49iu7hh
    image_id: fd8ia7s3s0srh3h52l8o
    s3_link: yc-marketplace-tested-images/nat-instance.qcow2
    sealed_image_id: fd85ln370r540mlk1gcn
state: active
tags: []
terms_of_service:
- translations:
    en:
      title: CloudIL Marketplace Terms of Service
    ru:
      title: Условиями использования CloudIL Marketplace
  type: default-tos
  url: https://cloudil.co.il/legal/cloud_terms_marketplace/
updated_at: 1653907478
verified_text: true`)
	var versionYaml model.VersionYaml
	_ = yaml.Unmarshal(input, &versionYaml)

	result := createVersionSyncRequest(versionYaml)

	suite.Require().Empty(result.Payload.ComputeImage.ResourceSpec.GPU)
	suite.Require().Empty(result.Payload.ComputeImage.ResourceSpec.DiskSize.Recommended)
	suite.Require().Equal(1, *result.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.Min)
	suite.Require().Equal(8, *result.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.Max)
	suite.Require().Empty(result.Payload.ComputeImage.ResourceSpec.NetworkInterfaces.Recommended)
}

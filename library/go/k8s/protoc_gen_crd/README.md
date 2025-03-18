protoc-gen-crd
==============

This is a protobuf compiler plugin to generate Kubernetes YAML spec for CRD from protobuf definition.

## Usage
 
First, you need to write the protobuf spec with the CRD annotations:

```protobuf
import "library/go/k8s/protoc_gen_crd/proto/crd.proto";

message Spec {
    // your fields
}

message Status {
    // your fields
}

message MyCrdKind {
    option (protoc_gen_crd.k8s_crd) = {
           api_group: "my-api.yandex-team.ru",
           kind: "MyCrdKind",
           plural: "mycrdkinds",
           singular: "mycrdkind",
    };

    Spec spec = 1;
    Status status = 2;
}

```

Then add the following to `ya.make` of your PROTO_LIBRARY:

```
INCLUDE(${ARCADIA_ROOT}/library/go/k8s/protoc_gen_crd/build.inc)
```

Finally you build it:
```bash
ya make --add-result .crd.yaml --replace-result
```

This will give you `<filename>.crd.yaml`, which can be put with `kubectl apply -f <filename>.crd.yaml`.

## Kustomize patch hints

Optionally you can specify kustomize patch parameters via special annotation:

```protobuf
message Spec {
    repeated MyField my_fields = 1 [(protoc_gen_crd.k8s_patch) = {
        merge_key: "some_key",
        merge_strategy: "merge",
    }];
}
```

## Usage in k8s controllers

If you are writing k8s CRD controller in Go, you might also want to include deepcopy and doc generators for kubebuilder in your `ya.make`:
```
INCLUDE(${ARCADIA_ROOT}/yt/yt/orm/go/codegen/proto-comments/build.inc)
INCLUDE(${ARCADIA_ROOT}/yp/go/protoc-gen-deepcopy/build.inc)
```

The contents of `MyCrdKind` should mimic the corresponding Go struct that would be registered in controller:

```go
//+kubebuilder:object:root=true
type MyCrdKind struct {
	metav1.TypeMeta   `json:",inline"`
	metav1.ObjectMeta `json:"metadata,omitempty"`

	Spec   *protobuf_v1.Spec   `json:"spec,omitempty"`
	Status *protobuf_v1.Status `json:"status,omitempty"`
}
```

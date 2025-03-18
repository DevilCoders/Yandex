package main

import (
	"flag"
	"fmt"
	"log"
	"regexp"
	"strings"

	"github.com/google/gnostic/compiler"
	v3 "github.com/google/gnostic/openapiv3"
	"google.golang.org/protobuf/compiler/protogen"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
	"gopkg.in/yaml.v3"

	crd "a.yandex-team.ru/library/go/k8s/protoc_gen_crd/proto"
)

const (
	patchMergeKeyField      = "x-kubernetes-patch-merge-key"
	patchMergeStrategyField = "x-kubernetes-patch-strategy"

	intOrStringField           = "x-kubernetes-int-or-string"
	preserveUnknownFieldsField = "x-kubernetes-preserve-unknown-fields"
)

var (
	columnTypeName = map[crd.ColumnType]string{
		crd.ColumnType_CT_INTEGER: "integer",
		crd.ColumnType_CT_NUMBER:  "number",
		crd.ColumnType_CT_STRING:  "string",
		crd.ColumnType_CT_BOOLEAN: "boolean",
		crd.ColumnType_CT_DATE:    "date",
	}
	columnFormatName = map[crd.ColumnFormat]string{
		crd.ColumnFormat_CF_INT32:    "int32",
		crd.ColumnFormat_CF_INT64:    "int64",
		crd.ColumnFormat_CF_FLOAT:    "float",
		crd.ColumnFormat_CF_DOUBLE:   "double",
		crd.ColumnFormat_CF_BYTE:     "byte",
		crd.ColumnFormat_CF_DATE:     "date",
		crd.ColumnFormat_CF_DATETIME: "date-time",
		crd.ColumnFormat_CF_PASSWORD: "password",
	}
)

var flags flag.FlagSet
var intOrStringExtension = &v3.NamedAny{
	Name:  intOrStringField,
	Value: &v3.Any{Yaml: "true"},
}
var preserveUnknownFieldsExtension = &v3.NamedAny{
	Name:  preserveUnknownFieldsField,
	Value: &v3.Any{Yaml: "true"},
}

var opaqueSchema = &v3.SchemaOrReference{
	Oneof: &v3.SchemaOrReference_Schema{Schema: &v3.Schema{
		Type:                   "object",
		SpecificationExtension: []*v3.NamedAny{preserveUnknownFieldsExtension},
	}},
}

type Schema struct {
	visitedSchemas map[string]struct{}
	typesStack     map[string]bool
	schemas        *v3.SchemasOrReferences
	metadata       *crd.K8SCRD

	linterRulePattern *regexp.Regexp
}

func fullMessageTypeName(message protoreflect.MessageDescriptor) string {
	return "." + string(message.ParentFile().Package()) + "." + string(message.Name())
}

func (s *Schema) formatFieldName(field *protogen.Field) string {
	return string(field.Desc.Name())
}

func (s *Schema) formatMessageName(message *protogen.Message) string {
	return string(message.Desc.Name())
}

func (s *Schema) formatMessageRef(name string) string {
	return name
}

func (s *Schema) OneCrd() bool {
	return len(s.schemas.AdditionalProperties) == 1
}

func (s *Schema) getPatchAnnotation(message *protogen.Field) *crd.K8SPatch {
	if message == nil {
		return nil
	}
	xt := crd.E_K8SPatch
	extension := proto.GetExtension(message.Desc.Options(), xt)
	if extension == nil || extension == xt.InterfaceOf(xt.Zero()) {
		return nil
	}
	return extension.(*crd.K8SPatch)

}

func (s *Schema) makeSpecificationExtension(patchAnnotation *crd.K8SPatch) []*v3.NamedAny {
	var out []*v3.NamedAny
	if patchAnnotation == nil {
		return out
	}
	if len(patchAnnotation.MergeKey) > 0 {
		out = append(out, &v3.NamedAny{
			Name: patchMergeKeyField,
			Value: &v3.Any{
				Yaml: patchAnnotation.MergeKey,
			},
		})
	}
	if len(patchAnnotation.MergeStrategy) > 0 {
		out = append(out, &v3.NamedAny{
			Name: patchMergeStrategyField,
			Value: &v3.Any{
				Yaml: patchAnnotation.MergeStrategy,
			},
		})
	}
	return out
}

func (s *Schema) shouldVisitSchema(typeName string) bool {
	_, ok := s.visitedSchemas[typeName]
	if ok {
		return false
	}
	s.visitedSchemas[typeName] = struct{}{}
	return true
}

func (s *Schema) filterCommentString(c protogen.Comments, removeNewLines bool) string {
	comment := string(c)
	if removeNewLines {
		comment = strings.Replace(comment, "\n", "", -1)
	}
	comment = s.linterRulePattern.ReplaceAllString(comment, "")
	return strings.TrimSpace(comment)
}

func (s *Schema) schemaReferenceForTypeName(typeName string) string {
	parts := strings.Split(typeName, ".")
	lastPart := parts[len(parts)-1]
	return "#/components/schemas/" + s.formatMessageRef(lastPart)
}

func (s *Schema) schemaOrReferenceForTypeOrMessage(typeName string, message *protogen.Message) *v3.SchemaOrReference {
	switch typeName {

	// TODO (torkve) Create oneof here: we probably should allow user to provide either formatted string (RFC3339 etc)
	//	             or proto-compatible struct: to support direct passing objects from dctl.
	//               But gnostic currently doesn't support Type to be an array.

	case ".google.protobuf.Timestamp":
		// Timestamps are serialized as strings
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "string", Format: "RFC3339"}}}

	case ".google.type.Date":
		// Dates are serialized as strings
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "string", Format: "date"}}}

	case ".google.type.DateTime":
		// DateTimes are serialized as strings
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "string", Format: "date-time"}}}

	case ".google.protobuf.Struct":
		// Struct is equivalent to a JSON object
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "object"}}}

	case ".google.protobuf.Empty":
		// Empty is close to JSON undefined than null, so ignore this field
		return nil //&v3.SchemaOrReference{Oneof: &v3.SchemaOrReference_Schema{Schema: &v3.Schema{Type: "null"}}}

	case ".google.protobuf.Any":
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{
					Type:     "object",
					Nullable: true,
					Properties: &v3.Properties{AdditionalProperties: []*v3.NamedSchemaOrReference{
						{
							Name: "@type",
							Value: &v3.SchemaOrReference{Oneof: &v3.SchemaOrReference_Schema{
								Schema: &v3.Schema{Type: "string"},
							}},
						},
					}},
					Required: []string{"@type"},
					AdditionalProperties: &v3.AdditionalPropertiesItem{Oneof: &v3.AdditionalPropertiesItem_Boolean{
						Boolean: true,
					}},
					SpecificationExtension: []*v3.NamedAny{preserveUnknownFieldsExtension},
				},
			},
		}
	default:
		return s.schemaForMessage(message, false)
	}
}

func (s *Schema) schemaOrReferenceForField(field *protogen.Field) *v3.SchemaOrReference {
	patchAnnotation := s.getPatchAnnotation(field)

	if field.Desc.IsMap() {
		mapMessage := field.Message.Fields[1]
		return &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "object",
					AdditionalProperties: &v3.AdditionalPropertiesItem{
						Oneof: &v3.AdditionalPropertiesItem_SchemaOrReference{
							SchemaOrReference: s.schemaOrReferenceForField(mapMessage),
						},
					},
					SpecificationExtension: s.makeSpecificationExtension(patchAnnotation),
				},
			},
		}
	}

	var kindSchema *v3.SchemaOrReference

	fieldDescription := s.filterCommentString(field.Comments.Leading, true)

	kind := field.Desc.Kind()

	switch kind {

	case protoreflect.MessageKind:
		typeName := fullMessageTypeName(field.Desc.Message())
		kindSchema = s.schemaOrReferenceForTypeOrMessage(typeName, field.Message)
		if kindSchema == nil {
			return nil
		}

	case protoreflect.StringKind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "string", Description: fieldDescription}}}

	case protoreflect.Int32Kind, protoreflect.Sint32Kind, protoreflect.Uint32Kind,
		protoreflect.Sfixed32Kind, protoreflect.Fixed32Kind, protoreflect.Sfixed64Kind,
		protoreflect.Fixed64Kind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "integer", Format: kind.String(), Description: fieldDescription}}}
	case protoreflect.Int64Kind, protoreflect.Sint64Kind, protoreflect.Uint64Kind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{
					Format:                 kind.String(),
					SpecificationExtension: []*v3.NamedAny{intOrStringExtension},
					Description:            fieldDescription,
				},
			},
		}
	case protoreflect.EnumKind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{
					Format:                 "enum",
					SpecificationExtension: []*v3.NamedAny{intOrStringExtension},
					Description:            fieldDescription,
				},
			},
		}

	case protoreflect.BoolKind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{
					Type:        "boolean",
					Description: fieldDescription,
				}}}

	case protoreflect.FloatKind, protoreflect.DoubleKind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "number", Format: kind.String(), Description: fieldDescription}}}

	case protoreflect.BytesKind:
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{Type: "string", Format: "bytes", Description: fieldDescription}}}

	default:
		log.Printf("(TODO) Unsupported field type: %+v", fullMessageTypeName(field.Desc.Message()))
	}

	if field.Desc.IsList() {
		kindSchema = &v3.SchemaOrReference{
			Oneof: &v3.SchemaOrReference_Schema{
				Schema: &v3.Schema{
					Type:  "array",
					Items: &v3.ItemsItem{SchemaOrReference: []*v3.SchemaOrReference{kindSchema}},
				},
			},
		}
	}

	if schema := kindSchema.GetSchema(); patchAnnotation != nil && schema != nil {
		schema.SpecificationExtension = append(schema.SpecificationExtension, s.makeSpecificationExtension(patchAnnotation)...)
	}

	return kindSchema
}

func (s *Schema) schemaForMessage(message *protogen.Message, isRoot bool) *v3.SchemaOrReference {
	typename := fullMessageTypeName(message.Desc)

	if s.typesStack[typename] {
		return opaqueSchema
	}

	messageDescription := s.filterCommentString(message.Comments.Leading, true)
	definitionProperties := &v3.Properties{
		AdditionalProperties: make([]*v3.NamedSchemaOrReference, 0),
	}

	s.typesStack[typename] = true
	defer delete(s.typesStack, typename)

	// TODO (torkve) process oneof's separately

	for _, field := range message.Fields {
		// The field is either described by a reference or a schema.
		fieldSchema := s.schemaOrReferenceForField(field)
		if fieldSchema == nil {
			continue
		}

		if schema, ok := fieldSchema.Oneof.(*v3.SchemaOrReference_Schema); ok {
			// Get the field description from the comments.
			schema.Schema.Description = s.filterCommentString(field.Comments.Leading, true)
		}

		definitionProperties.AdditionalProperties = append(
			definitionProperties.AdditionalProperties,
			&v3.NamedSchemaOrReference{
				Name:  s.formatFieldName(field),
				Value: fieldSchema,
			},
		)
	}
	return &v3.SchemaOrReference{
		Oneof: &v3.SchemaOrReference_Schema{
			Schema: &v3.Schema{
				Type:        "object",
				Nullable:    !isRoot,
				Description: messageDescription,
				Properties:  definitionProperties,
			},
		},
	}
}

func (s *Schema) addSchemas(messages []*protogen.Message) {
	for _, message := range messages {
		if message.Messages != nil {
			s.addSchemas(message.Messages)
		}

		typeName := fullMessageTypeName(message.Desc)
		if !s.shouldVisitSchema(typeName) {
			continue
		}

		xt := crd.E_K8SCrd
		extension := proto.GetExtension(message.Desc.Options(), xt)
		if extension == nil || extension == xt.InterfaceOf(xt.Zero()) {
			continue
		}
		s.metadata = extension.(*crd.K8SCRD)

		s.schemas.AdditionalProperties = append(s.schemas.AdditionalProperties,
			&v3.NamedSchemaOrReference{
				Name:  s.formatMessageName(message),
				Value: s.schemaForMessage(message, true),
			},
		)
	}
}

func renderAdditionalColumn(column *crd.PrinterColumn) *yaml.Node {
	node := compiler.NewMappingNode()

	node.Content = append(node.Content, compiler.NewScalarNodeForString("name"))
	node.Content = append(node.Content, compiler.NewScalarNodeForString(column.Name))

	if column.Type != crd.ColumnType_CT_NONE {
		node.Content = append(node.Content, compiler.NewScalarNodeForString("type"))
		node.Content = append(node.Content, compiler.NewScalarNodeForString(columnTypeName[column.Type]))
	}

	if column.Format != crd.ColumnFormat_CF_NONE {
		node.Content = append(node.Content, compiler.NewScalarNodeForString("format"))
		node.Content = append(node.Content, compiler.NewScalarNodeForString(columnFormatName[column.Format]))
	}

	if column.Description != "" {
		node.Content = append(node.Content, compiler.NewScalarNodeForString("description"))
		node.Content = append(node.Content, compiler.NewScalarNodeForString(column.Description))
	}

	node.Content = append(node.Content, compiler.NewScalarNodeForString("jsonPath"))
	node.Content = append(node.Content, compiler.NewScalarNodeForString(column.JsonPath))

	if column.Priority != 0 {
		node.Content = append(node.Content, compiler.NewScalarNodeForString("priority"))
		node.Content = append(node.Content, compiler.NewScalarNodeForInt(int64(column.Priority)))
	}

	return node
}

func main() {
	opts := protogen.Options{
		ParamFunc: flags.Set,
	}

	opts.Run(func(plugin *protogen.Plugin) error {
		for _, file := range plugin.Files {
			schema := &Schema{
				visitedSchemas:    make(map[string]struct{}),
				typesStack:        map[string]bool{},
				schemas:           &v3.SchemasOrReferences{AdditionalProperties: make([]*v3.NamedSchemaOrReference, 0)},
				linterRulePattern: regexp.MustCompile(`\(-- .* --\)`),
			}
			schema.addSchemas(file.Messages)

			if !schema.OneCrd() {
				continue
			}

			header := compiler.NewMappingNode()
			header.Content = append(header.Content, compiler.NewScalarNodeForString("apiVersion"))
			header.Content = append(header.Content, compiler.NewScalarNodeForString("apiextensions.k8s.io/v1"))

			header.Content = append(header.Content, compiler.NewScalarNodeForString("kind"))
			header.Content = append(header.Content, compiler.NewScalarNodeForString("CustomResourceDefinition"))

			metadata := compiler.NewMappingNode()

			metadata.Content = append(metadata.Content, compiler.NewScalarNodeForString("name"))
			metadata.Content = append(metadata.Content, compiler.NewScalarNodeForString(schema.metadata.Plural+"."+schema.metadata.ApiGroup))

			metadata.Content = append(metadata.Content, compiler.NewScalarNodeForString("annotations"))
			metadata.Content = append(metadata.Content, compiler.NewMappingNode())

			header.Content = append(header.Content, compiler.NewScalarNodeForString("metadata"))
			header.Content = append(header.Content, metadata)

			spec := compiler.NewMappingNode()
			spec.Content = append(spec.Content, compiler.NewScalarNodeForString("group"))
			spec.Content = append(spec.Content, compiler.NewScalarNodeForString(schema.metadata.ApiGroup))

			spec.Content = append(spec.Content, compiler.NewScalarNodeForString("scope"))
			spec.Content = append(spec.Content, compiler.NewScalarNodeForString("Namespaced")) // TODO (torkve) pass as metadata field

			names := compiler.NewMappingNode()

			names.Content = append(names.Content, compiler.NewScalarNodeForString("kind"))
			names.Content = append(names.Content, compiler.NewScalarNodeForString(schema.metadata.Kind))

			names.Content = append(names.Content, compiler.NewScalarNodeForString("listKind"))
			names.Content = append(names.Content, compiler.NewScalarNodeForString(schema.metadata.Kind+"List"))

			names.Content = append(names.Content, compiler.NewScalarNodeForString("plural"))
			names.Content = append(names.Content, compiler.NewScalarNodeForString(schema.metadata.Plural))

			names.Content = append(names.Content, compiler.NewScalarNodeForString("singular"))
			names.Content = append(names.Content, compiler.NewScalarNodeForString(schema.metadata.Singular))

			names.Content = append(names.Content, compiler.NewScalarNodeForString("shortNames"))
			names.Content = append(names.Content, compiler.NewSequenceNodeForStringArray(schema.metadata.ShortNames))

			names.Content = append(names.Content, compiler.NewScalarNodeForString("categories"))
			names.Content = append(names.Content, compiler.NewSequenceNodeForStringArray(schema.metadata.Categories))

			spec.Content = append(spec.Content, compiler.NewScalarNodeForString("names"))
			spec.Content = append(spec.Content, names)

			versions := compiler.NewSequenceNode()

			versionV1 := compiler.NewMappingNode()
			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("name"))
			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("v1")) // TODO (torkve) support multiple versions and custom naming

			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("served"))
			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForBool(true))

			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("storage"))
			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForBool(true))

			subresources := compiler.NewMappingNode()
			// FIXME (torkve) currently we are enforcing exactly one subresource named "status". It must be
			//                configurable via field annotations
			subresources.Content = append(subresources.Content, compiler.NewScalarNodeForString("status"))
			subresources.Content = append(subresources.Content, compiler.NewMappingNode())

			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("subresources"))
			versionV1.Content = append(versionV1.Content, subresources)

			versionV1Schema := compiler.NewMappingNode()
			versionV1Schema.Content = append(versionV1Schema.Content, compiler.NewScalarNodeForString("openAPIV3Schema"))
			versionV1Schema.Content = append(versionV1Schema.Content, schema.schemas.ToRawInfo().Content[1])

			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("schema"))
			versionV1.Content = append(versionV1.Content, versionV1Schema)

			additionalColumns := compiler.NewSequenceNode()
			for _, column := range schema.metadata.AdditionalColumns {
				additionalColumns.Content = append(additionalColumns.Content, renderAdditionalColumn(column))
			}

			versionV1.Content = append(versionV1.Content, compiler.NewScalarNodeForString("additionalPrinterColumns"))
			versionV1.Content = append(versionV1.Content, additionalColumns)

			versions.Content = append(versions.Content, versionV1)

			spec.Content = append(spec.Content, compiler.NewScalarNodeForString("versions"))
			spec.Content = append(spec.Content, versions)

			header.Content = append(header.Content, compiler.NewScalarNodeForString("spec"))
			header.Content = append(header.Content, spec)

			rawInfo := &yaml.Node{
				Kind:        yaml.DocumentNode,
				Style:       0,
				Content:     []*yaml.Node{header},
				HeadComment: "Generated by protoc-gen-crd from " + file.GeneratedFilenamePrefix + ".proto",
			}

			outputFileName := file.GeneratedFilenamePrefix + ".crd.yaml"
			outputFile := plugin.NewGeneratedFile(outputFileName, "")
			e := yaml.NewEncoder(outputFile)
			e.SetIndent(2)
			if err := e.Encode(rawInfo); err != nil {
				return fmt.Errorf("failed to marshal yaml: %s", err.Error())
			}
		}

		return nil
	})
}

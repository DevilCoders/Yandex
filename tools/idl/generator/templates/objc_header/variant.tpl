{{>DOCS}}YRT_EXPORT @interface {{TYPE_NAME}} : NSObject{{#TYPES}}

@property (nonatomic, readonly, nullable) {{OPTIONAL_CLASS_TYPE}}{{FIELD_NAME}};{{/TYPES}}
{{#TYPES}}
+ (nonnull {{TYPE_NAME}} *){{INSTANCE_NAME}}With{{FIELD_NAME:x-cap}}:({{#NON_OPTIONAL}}nonnull {{/NON_OPTIONAL}}{{CLASS_TYPE}}){{FIELD_NAME}};
{{/TYPES}}
@end

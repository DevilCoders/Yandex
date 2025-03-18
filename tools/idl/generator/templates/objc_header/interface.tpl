{{#NON_FWD_DECL_ABLE_TYPE}}{{>NON_FWD_DECL_ABLE_TYPE}}

{{/NON_FWD_DECL_ABLE_TYPE}}{{>DOCS}}YRT_EXPORT @interface {{TYPE_NAME}} : {{#PARENT}}{{NAME}}{{/PARENT}}{{#CHILD}}

{{>CHILD}}{{/CHILD}}{{#ITEM}}{{#METHOD}}

{{>METHOD}}{{/METHOD}}{{#PROPERTY}}
{{>PROPERTY_DOCS}}@property (nonatomic{{#READONLY}}, readonly{{/READONLY}}{{#INTERFACE}}, readonly{{/INTERFACE}}{{#OPTIONAL}}, nullable{{/OPTIONAL}}{{#NON_OPTIONAL}}, nonnull{{/NON_OPTIONAL}}{{#BOOL}}, getter=is{{PROPERTY_NAME:x-cap}}{{/BOOL}}) {{PROPERTY_TYPE}}{{#POD}} {{/POD}}{{#LISTENER}} {{/LISTENER}}{{PROPERTY_NAME}};{{/PROPERTY}}{{/ITEM}}{{#NO_PARENT}}{{#WEAK_INTERFACE}}

/**
 * Tells if this object is valid or no. Any method called on an invalid
 * object will throw an exception. The object becomes invalid only on UI
 * thread, and only when its implementation depends on objects already
 * destroyed by now. Please refer to general docs about the interface for
 * details on its invalidation.
 */
@property (nonatomic, readonly, getter=isValid) BOOL valid;{{/WEAK_INTERFACE}}{{/NO_PARENT}}

@end
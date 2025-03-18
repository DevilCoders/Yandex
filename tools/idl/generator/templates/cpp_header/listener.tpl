{{#EXCLUDE}}/// @cond EXCLUDE
{{/EXCLUDE}}{{>DOCS}}class YANDEX_EXPORT {{CONSTRUCTOR_NAME}} {{#BASE_LISTENER}}: public {{PARENT_CLASS}} {{/BASE_LISTENER}}{
public:
    virtual ~{{CONSTRUCTOR_NAME}}()
    {
    }{{#METHOD}}

    {{>METHOD}}{{/METHOD}}
};{{#EXCLUDE}}
/// @endcond{{/EXCLUDE}}
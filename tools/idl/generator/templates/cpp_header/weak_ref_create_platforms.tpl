{{#CREATE_PLATFORM}}{{#TARGET_VISIBLE}}
#ifdef BUILDING_FOR_TARGET
YANDEX_EXPORT boost::any createPlatform(const std::shared_ptr<{{TYPE_NAME}}>& {{INSTANCE_NAME}});
#else{{/TARGET_VISIBLE}}
inline boost::any createPlatform(const std::shared_ptr<{{TYPE_NAME}}>& /* {{INSTANCE_NAME}} */)
{
    ASSERT(false);
    return nullptr;
}{{#TARGET_VISIBLE}}
#endif
{{/TARGET_VISIBLE}}{{/CREATE_PLATFORM}}
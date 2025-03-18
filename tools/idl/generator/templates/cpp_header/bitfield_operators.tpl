{{#OPERATOR}}
inline {{ENUM_SCOPED_NAME}} operator|({{ENUM_SCOPED_NAME}} a, {{ENUM_SCOPED_NAME}} b)
{
    return static_cast<{{ENUM_SCOPED_NAME}}>(
        static_cast<unsigned int>(a) | static_cast<unsigned int>(b)
    );
}

inline {{ENUM_SCOPED_NAME}}& operator|=({{ENUM_SCOPED_NAME}}& a, {{ENUM_SCOPED_NAME}} b)
{
    return a = a | b;
}

inline {{ENUM_SCOPED_NAME}} operator&({{ENUM_SCOPED_NAME}} a, {{ENUM_SCOPED_NAME}} b)
{
    return static_cast<{{ENUM_SCOPED_NAME}}>(
        static_cast<unsigned int>(a) & static_cast<unsigned int>(b)
    );
}

inline {{ENUM_SCOPED_NAME}}& operator&=({{ENUM_SCOPED_NAME}}& a, {{ENUM_SCOPED_NAME}} b)
{
    return a = a & b;
}
{{/OPERATOR}}
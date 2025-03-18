#pragma once

#include <util/generic/string.h>

/// @}

/// @addtogroup Strings_FormattedText
/// @{
/// Преобразует текст в XML-код.
/// @details Символы, запрещенные спецификацией XML 1.0, удаляются.
TString EncodeXML(const char* str, int qEncodeEntities = 1);

/// Преобразует текст в XML-код, в котором могут присутствовать только цифровые сущности.
/// @details Cимволы, запрещенные спецификацией XML 1.0, не удаляются.
/// См. также @c EncodeXMLString(const TString& str);
TString EncodeXMLString(const char* str);

/// Преобразует текст в XML-код, в котором могут присутствовать только цифровые сущности.
/// @details Cимволы, запрещенные спецификацией XML 1.0, не удаляются.
TString EncodeXMLString(const TString& str);

/// @}

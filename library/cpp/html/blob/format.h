#pragma once

#include <util/system/defaults.h>

namespace NHtml {
    namespace NBlob {
        /**
         * Структура файла
         * (FRAMES)* | (STRINGS)* | META
         *
         * Структура узлов dom-дерева
         * Element
         *    flags | tag-id | [view] | [attributes] | [matched] | [computed]
         *
         * Text
         *   flags  | value-id
         *
         * Comment
         *    flags | value-id
         *
         */

        static const ui8 NODE_CLOSE = 0;
        static const ui8 NODE_ELEMENT = 1;
        static const ui8 NODE_TEXT = 2;
        static const ui8 NODE_COMMENT = 3;
        static const ui8 NODE_DOCUMENT = 4;
        static const ui8 NODE_DOCUMENT_TYPE = 5;

        static const ui8 NODE_TYPE_MASK = 0x07;

        static const ui8 FIELD_VIEW = 0x08;
        static const ui8 FIELD_ATTRIBUTES = 0x10;
        static const ui8 FIELD_MATCHED = 0x20;
        static const ui8 FIELD_COMPUTED = 0x40;
        //! Все вышезаданные поля имеют фиксированный формат представления данных.
        //! При добавлении нового поля надо обеспечить обратную совместимость.
        //! Для этого весь блок расширений оформляется как length-delimited поле.
        static const ui8 FIELD_EXTENSIONS = 0x80;

    }
}

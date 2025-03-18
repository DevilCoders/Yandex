#pragma once

#include <library/cpp/html/spec/tags.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NHtml {
    namespace NBlob {
        //! Координаты габаритного прямоугольника.
        struct TRect {
            i32 X = 0;
            i32 Y = 0;
            i32 Width = 0;
            i32 Height = 0;
        };

        struct TElement {
            struct TPair {
                TString Name;
                TString Value;
            };

            //! Имя элемента.
            TString Name;
            //! Предопределённый тип элемента.
            const TTag* Tag;
            //! Габаритные размеры элемента.
            //!
            //! Все размеры элементов всегда возвращаются в масштабе 1.0.
            //! В случае, если пользователь видит мастабированную версию документа,
            //! для получения истинных габаритов элемента, надо каждое значение
            //! умножить на ScaleFactor.
            TRect Viewbound;
            //! Список атрибутов элемента.
            TVector<TPair> Attributes;
            //! Сопоставленные стили элемента.
            TVector<TPair> MatchedStyle;
            //! Вычисленные стили элемента.
            TVector<TPair> ComputedStyle;
        };

        //! Класс, позволяющий получить события обхода dom-дерева.
        class INodeVisitor {
        public:
            virtual ~INodeVisitor() = default;

            //! Вызывается однократно в начале обхода документа.
            virtual void OnDocumentStart() = 0;

            //! Вызывается однократно в конце обхода документа.
            virtual void OnDocumentEnd() = 0;

            //! Элемент doctype.
            virtual void OnDocumentType(const TString& doctype) = 0;

            //! Вход в html-тег.
            virtual void OnElementStart(const TElement& elem) = 0;

            //! Вход из html-тега.  Вызывается даже для empty-tags таких, например,
            //! как <meta>.
            virtual void OnElementEnd(const TString& name) = 0;

            //! Текстовый элемент.
            //! Текстовые элементы не имеют собственных габаритных размеров; для этого
            //! надо ориентироваться на родительский элемент.
            virtual void OnText(const TString& text) = 0;

            //! Комментарий.
            virtual void OnComment(const TString& text) = 0;
        };

    }
}

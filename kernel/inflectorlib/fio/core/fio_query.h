#pragma once

#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>

class TLanguage;

namespace NFioInflector {
    // Формат запроса:
    // persn{name}famn{surname}gender{m/f}opts{...}
    // Поля могут идти в произвольном порядке. Если пол не указан в запросе,
    // то стараемся его угадать автоматически.
    //
    // И name, и surname могут быть строками любого вида и содержать любую пунктуацию
    // кроме разделителя полей ({}). Пунктуация и исходная разметка сохраняется.
    // Склоняются только поля persn и famn. Остальные поля остаются без изменений.
    //
    // В поле opts можно указать дополнительные флаги через запятую. Допустимые значения:
    //     advnorm - включить advanced нормализацию имен (BEGEMOT-949)
    //
    // Example:
    // lang = TNewRussianFIOLanguage::GetLang();
    // query = "persn{Эрих Мария}famn{Ремарк}gender{m}"
    // EGrammar cases[] = { gNominative, gGenitive, gDative, gAccusative, gInstrumental, gAblative, gInvalid };
    //
    // persn{Эрих Мария}famn{Ремарк}gender{m}
    // persn{Эриха Марии}famn{Ремарка}gender{m}
    // persn{Эриху Марии}famn{Ремарку}gender{m}
    // persn{Эриха Марию}famn{Ремарка}gender{m}
    // persn{Эрихом Марией}famn{Ремарком}gender{m}
    // persn{Эрихе Марии}famn{Ремарке}gender{m}
    TVector<TUtf16String> Inflect(const TLanguage* lang, const TUtf16String& query, const EGrammar cases[]);

    template<typename Lang>
    TVector<TUtf16String> Inflect(const TUtf16String& query, const EGrammar cases[]) {
        const TLanguage* lang = Lang::GetLang();
        return Inflect(lang, query, cases);
    }

    bool HasMarkup(const TUtf16String& query);
}

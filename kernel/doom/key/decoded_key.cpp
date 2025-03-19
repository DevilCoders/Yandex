#include "decoded_key.h"

namespace NDoom {


IOutputStream& operator<<(IOutputStream& stream, const TDecodedKey& key) {
    stream << key.Lemma();

    /* Don't add forms in case that's a single form key. */
    if (key.FormCount() == 1) {
        const TDecodedFormRef& form = key.Form(0);
        if (!form.Flags() && form.Language() == LANG_UNK && form.Text() == key.Lemma())
            return stream;
    }

    stream << " (";
    for (size_t i = 0; i < key.FormCount(); i++) {
        if (i != 0)
            stream << ", ";

        const TDecodedFormRef& form = key.Form(i);

        if (form.Flags() & (FORM_TITLECASE | FORM_TRANSLIT | FORM_HAS_JOINS)) {
            stream << "[";
            if (form.Flags() & FORM_TITLECASE)
                stream << "U";
            if (form.Flags() & FORM_TRANSLIT)
                stream << "T";
            stream << "]";
        }

        stream << form.Text();

        if (form.Language() != LANG_UNK) {
            stream << "{";
            stream << IsoNameByLanguage(form.Language());
            stream << "}";
        }
    }
    stream << ")";
    return stream;
}

} // namespace NDoom

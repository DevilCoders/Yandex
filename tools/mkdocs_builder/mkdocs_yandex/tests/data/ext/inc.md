# Something about inclusion

## Literal md include

{% code 'includes/one.md' %}

## Literal code include

{% code "includes/code.py" %}

## Rendered md include

{% include "includes/one.md" %}

Check that simple [links](inc_note.md) still resolve correctly after rendered include.

## Inline rendered include

XXX {% include 'includes/string.md' %} XXX

## Markdown-style link to included file

{% include "[Title](includes/one.md)" %}

Check that simple [links](inc_note.md) still resolve correctly after rendered include with markdown-style link.

With no qoutes:

{% include [Title](includes/one.md) %}

Check that simple [links](inc_note.md) still resolve correctly after rendered include with markdown-style link and no quotes.

Unicode title:

{% include "[Текст](includes/one.md)" %}

Check that simple (though fake) ![image links](inc_note.md) still resolve correctly after all.

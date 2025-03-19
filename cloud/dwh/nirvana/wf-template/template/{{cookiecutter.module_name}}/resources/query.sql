{% raw -%}
$src_table = {{ param["source_path"] -> quote() }};
$dst_table = {{ input1 -> table_quote() }};
{% endraw -%}

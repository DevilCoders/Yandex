-- {{ file }}
$d1 = '{{ start.strftime(table_name_fmt) }}';
$d2 = '{{ end.strftime(table_name_fmt) }}';
$table = '{{ train_dataset_path }}' || '/' || $d1 || '_' || $d2;
select {{ ", ".join(cat_features + float_features + target_features + pass_through) }}
from $table {% if percent < 100 %} TABLESAMPLE BERNOULLI({{ percent }}) REPEATABLE(777) {% endif %}
order by {{ pass_through[0] }}

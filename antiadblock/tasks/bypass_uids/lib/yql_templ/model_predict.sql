-- {{ file }}
-- specify the URL where to get the model
PRAGMA file("model.dump", "https://proxy.sandbox.yandex-team.ru/last/ANTIADBLOCK_BYPASS_UIDS_MODEL");
$d1 = '{{ start.strftime(table_name_fmt) }}';
$d2 = '{{ end.strftime(table_name_fmt) }}';
$table = '{{ dataset_path }}' || '/' || $d1 || '_' || $d2;

$logr = ($raw_score) -> {return Math::Exp($raw_score)/(1 + Math::Exp($raw_score));};
-- Initialize CatBoost FormulaEvaluator with given model:
$evaluator = CatBoost::LoadModel(FilePath("model.dump"));
-- Prepare the data:
$features = (
  select {{ ", ".join(cat_features + float_features + pass_through) }}
  from (select * from $table where `{{ columns.adblock }}` = 0)
 {% if percent < 100 %} TABLESAMPLE BERNOULLI({{ percent }}) REPEATABLE(777) {% endif %}
 {% if device != devices.all.name %} where `{{ columns.device }}` = '{{ device }}' {% endif %}
);
$data = (
  SELECT
    [{{ ", ".join(cat_features) }}] AS CatFeatures,  -- Cat features are packed into List<String>
    [{{ ", ".join(float_features) }}] AS FloatFeatures,  -- Float features are packed into a List<Float>
    `{{ pass_through[0] }}` AS PassThrough
  FROM $features
);
$processed = (PROCESS $data USING CatBoost::EvaluateBatch($evaluator, TableRows()));
$predictions = (
  SELECT
    PassThrough as `{{ pass_through[0] }}`,
    $logr(Result[0]) as probability
  FROM $processed
);
$threshold = {{ threshold }};
insert into `{{ results_path }}` WITH TRUNCATE
select `{{ pass_through[0] }}`
from $predictions
where probability < $threshold
order by `{{ pass_through[0] }}`;
commit;

-- {{ file }}
-- specify the URL where to get the model
PRAGMA file("model.dump", "https://proxy.sandbox.yandex-team.ru/{{ resource_id }}");
$d1 = '{{ start.strftime(table_name_fmt) }}';
$d2 = '{{ end.strftime(table_name_fmt) }}';
$table = '{{ dataset_path }}' || '/' || $d1 || '_' || $d2;

$logr = ($raw_score) -> {return Math::Exp($raw_score)/(1 + Math::Exp($raw_score));};
-- Initialize CatBoost FormulaEvaluator with given model:
$evaluator = CatBoost::LoadModel(FilePath("model.dump"));
-- Prepare the data:
$features = (
  select {{ ", ".join(cat_features + float_features + target_features + pass_through) }}
  from $table
 {% if percent < 100 %} TABLESAMPLE BERNOULLI({{ percent }}) REPEATABLE(777) {% endif %}
 {% if device != devices.all.name %} where `{{ columns.device }}` = '{{ device }}' {% endif %}
);
$data = (
  SELECT
    [{{ ", ".join(cat_features) }}] AS CatFeatures,  -- Cat features are packed into List<String>
    [{{ ", ".join(float_features) }}] AS FloatFeatures,  -- Float features are packed into a List<Float>
    `{{ target_features[0] }}` AS PassThrough
  FROM $features
);
$processed = (PROCESS $data USING CatBoost::EvaluateBatch($evaluator, TableRows()));
$predictions = (
  SELECT
    PassThrough as answer,
    $logr(Result[0]) as probability
  FROM $processed
);
$threshold = {{ threshold }};
$perf = (
  select
    count(*) as total_rows,
    count_if(answer = 1) as positive_rows,
    count_if(answer = 1 and probability >= $threshold) as TP,
    count_if(answer = 1 and probability < $threshold) as FN,
    count_if(answer = 0 and probability >= $threshold) as FP,
    count_if(answer = 0 and probability < $threshold) as TN,
  from $predictions
);

select
  total_rows, positive_rows,
  TP, FN, FP, TN,
  TP * 100. / (TP + FN) as recall,
  (TP + TN) * 100. / total_rows as accuracy,
from $perf

UPDATE public.configs
SET data = jsonb_set(
    data,
    '{AD_SYSTEMS}',
    (data->'AD_SYSTEMS') - (
        SELECT ordinality - 1
        FROM jsonb_array_elements(data->'AD_SYSTEMS') WITH ordinality
        WHERE value = '3'
    )::int -- finds index of value to remove it (because json objects cant be removed using - operator)
    )
WHERE data->'AD_SYSTEMS' @> '3';

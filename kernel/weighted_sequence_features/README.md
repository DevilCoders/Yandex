## WeightedSequenceFeatures - библиотека для вычисления факторов по вектору пар <вес события, величина события>

Принципы:
1. Код состоит из *юнитов*. (привет текстовой машине)
1. Объединению юнитов определяет *виджет*.
1. Каждый юнит вычисляет счётчики.
1. Методы, вычисляющие факторы, смотрят в счётчики виджета.
1. Усли необходимого счётчика в виджете нет - падение компиляции.
1. Каждый юнит имеет уровень (от нуля и выше). юнит может иметь доступ только к счётчика с меньшим уровнем.
1. Юниты, из которых состоит виджет должны идти в порядке возрастания уровня
1. Ещё одна сущность - фильтры. Пока такой один - top5 фильтр.
1. Никаких аллокаций.

Голлосарий:
* weight - вес события
* feature - величина события

### Примеры
```
struct TSimpleWidged :
 public NWeighedSequenceFeatures::TWidget
<
    NWeighedSequenceFeatures::TUnitMaxWeihgt,
    NWeighedSequenceFeatures::TUnitMaxFeature,
    NWeighedSequenceFeatures::TUnitMaxWeightedFeature
>
{}

std::pair<float, float> Calcer(
    TArrayRef<float> weights,
    TArrayRef<float> features)
{
    TSimpleWidged widget; //no any allocations!
    widged.PrepareState(weights, features);

    return {
        NWeighedSequenceFeatures::CalcMaxWFNormedMaxW(widget),
        NWeighedSequenceFeatures::CalcMaxWF(widget)
    };
}
```


### TODO:
а) можно sse завезти (см NSeq4f)

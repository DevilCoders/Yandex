## Kafka recipe
This recipe allows to add Kafka queue to the suite objects. 

### Usage
Add the following include to the tests's ya.make:

    INCLUDE(${ARCADIA_ROOT}/library/recipes/kafka/recipe.inc)

Environmet varables are using:
    KAFKA_CREATE_TOPICS - coma separeted list of topics to create.
    KAFKA_TOPICS_PARTITIONS - partition count for each created topic (1 by default).

Sets environment variables:
    KAFKA_RECIPE_BROKER_LIST - broker list for use.


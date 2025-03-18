## Allure recipe
This recipe allows to add Allure-report to the suite objects. 

After the recipe is properly configured, the link to the report is appeared in the suite's links section at https://ci.yandex-team.ru

### Usage
Add 

Add the following include to the tests's ya.make:

    INCLUDE(${ARCADIA_ROOT}/library/recipes/allure/recipe.inc)
 

#### PY2TEST
If you're adding the recipe to pytest, add also PEERDIR to the allure lib to be able to use, allure report annotations:

    PEERDIR(library/python/pytest/allure)
    
The full example is available at: https://a.yandex-team.ru/arc/trunk/arcadia/library/recipes/allure/example/pytest

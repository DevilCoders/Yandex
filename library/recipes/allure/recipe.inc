
DEPENDS(
    jdk
    library/recipes/allure
    library/recipes/allure/allure_commandline
)

USE_RECIPE(
    library/recipes/allure/allure_recipe
)

TAG(
    ya:huge_logs
)

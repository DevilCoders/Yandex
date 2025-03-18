# SELENIUM UI TESTS
Library contains classes Drivers and Navigator with several methods to use in Selenium tests for UI. These methods could be used in your own UI tests as simple blocks.

Uses in Release Machine and Cores UI tests.

Maintainers: [g:release_machine](https://abc.yandex-team.ru/services/releasemachine/)


## Example

```python
for driver in selenium_ui_test.Drivers().drivers:
    with driver:
        navigator = selenium_ui_test.Navigator(
            main_url="your_main_url",
            driver=driver,
            results_folder="test_results",
            timeout=10,
        )
        wasted_time = navigator.check_text(url_path="main_page", text="Hello, world!")
        if wasted_time >= threshold:
            logging.error("Waited too long, %s seconds", wasted_time)
            # Send alert
```



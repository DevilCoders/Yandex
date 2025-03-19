from rich.console import ConsoleRenderable, RenderResult, Console, ConsoleOptions


class RunStatistics(ConsoleRenderable):
    def __init__(self):
        self.successes = 0
        self.fails = 0

    @property
    def total(self) -> int:
        return self.successes + self.fails

    def __rich_console__(
        self, console: Console, options: ConsoleOptions
    ) -> RenderResult:
        return console.render_str(
            f" [green3]Success: {self.successes}[/green3], [red3]fail: {self.fails}[/red3]"
        ).__rich_console__(console, options)

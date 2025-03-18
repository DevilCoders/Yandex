
class DefaultMyPyConfig(object):
    # use: MyPyConfig(DefaultMyPyConfig) inheritance
    follow_imports = 'normal'                           # --follow-imports=normal
    ignore_missing_imports = False                      # not set --ignore-missing-imports
    warn_unused_ignores = True                          # --warn-unused-ignores
    warn_return_any = False                             # not set --warn-return-any
    strict = False                                      # not set --strict
    show_traceback = False                              # not set --show-traceback
    # MyPy Reports https://mypy.readthedocs.io/en/stable/command_line.html#report-generation
    any_exprs_report = "testing_out_stuff/reports"      # how many expressions of type Any are present
    html_report = "testing_out_stuff/reports"           # generate an HTML type checking coverage report
    lineprecision_report = "testing_out_stuff/reports"  # generate a flat text file report with per-module statistics of how many lines are typechecked

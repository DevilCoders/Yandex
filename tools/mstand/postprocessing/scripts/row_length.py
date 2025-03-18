class RowLengthPostprocessor(object):
    # noinspection PyMethodMayBeStatic
    def process_experiment(self, exp):
        # noinspection PyProtectedMember
        with exp._source_tsv as rows:
            for row in rows:
                exp.write_value(len(row))

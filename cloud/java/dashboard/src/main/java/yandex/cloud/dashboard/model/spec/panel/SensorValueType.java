package yandex.cloud.dashboard.model.spec.panel;

import yandex.cloud.dashboard.model.spec.panel.QuerySpec.SelectBuilder;

/**
 * @author ssytnik
 */
public enum SensorValueType {
    raw {
        @Override
        public GroupByTimeSpec groupByTime(UseCase useCase) {
            switch (useCase) {
                case DERIV:
                    throw new IllegalArgumentException("Raw sensors cannot produce derivatives");
                case VALUE:
                case HIST_BUCKET:
                    return GroupByTimeSpec.SUM_DEFAULT;
                default:
                    throw new IllegalArgumentException();
            }
        }
    },
    rate {
        @Override
        public GroupByTimeSpec groupByTime(UseCase useCase) {
            // FIXME (CLOUD-40480): when 'integrate' is supported at SOLOMON, remove populate() and
            // return GroupByTimeSpec.INTEGRATE_DEFAULT;
            return GroupByTimeSpec.AVG_DEFAULT;
        }

        @Override
        SelectBuilder populate(SelectBuilder b, UseCase useCase) {
            return useCase == UseCase.VALUE ? b.addCall("integrate_fn").addCall("diff") : b;
        }
    },
    counter {
        @Override
        public GroupByTimeSpec groupByTime(UseCase useCase) {
            return GroupByTimeSpec.MAX_DEFAULT;
        }

        @Override
        SelectBuilder populate(SelectBuilder b, UseCase useCase) {
            return useCase == UseCase.VALUE ? b.addCall("diff").addCall("drop_below", "0") : b.addCall("nn_deriv");
        }
    };

    public abstract GroupByTimeSpec groupByTime(UseCase useCase);

    public SelectBuilder selectBuilder(UseCase useCase) {
        return populate(SelectBuilder.selectBuilder(), useCase);
    }

    SelectBuilder populate(SelectBuilder b, UseCase useCase) {
        return b;
    }

    public enum UseCase {
        DERIV,
        VALUE,
        HIST_BUCKET
    }
}
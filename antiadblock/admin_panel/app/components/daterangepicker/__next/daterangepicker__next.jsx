import React from 'react';
import PropTypes from 'prop-types';

import {Th} from 'app/components/table/table-components';
import Icon from 'lego-on-react/src/components/icon/icon.react';
import {dateLibDayJsType} from 'app/types';

export default class DaterangepickerNext extends React.Component {
    handleNext(calendar) {
        const {
            handleNext
        } = this.props;

        if (handleNext) {
            handleNext(calendar);
        }
    }

    render() {
        const {
            next,
            calendar
        } = this.props;

        const onClick = calendar && next ? this.handleNext.bind(this, calendar) : () => {};
        const nextProps = {
            onClick
        };

        return (
            <Th
                block='daterangepicker-table'
                elem='cell'
                mods={{
                    header: true,
                    next: true,
                    available: next
                }}
                {...nextProps}>
                {next ? <Icon
                    glyph='type-arrow'
                    direction='right' /> : null}
            </Th>
        );
    }
}

DaterangepickerNext.propTypes = {
    calendar: dateLibDayJsType.isRequired,
    next: PropTypes.bool,
    handleNext: PropTypes.func
};

DaterangepickerNext.defaultProps = {
    next: true
};

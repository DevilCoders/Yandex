import React from 'react';
import {shallow} from 'enzyme';
import toJson from 'enzyme-to-json';

import Error from 'app/components/error/error';

describe('error component', () => {
    test('without props', () => {
        const wrapper = shallow(<Error />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('with status and message', () => {
        const wrapper = shallow(<Error status={500} messageI18N='default' />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('with status and untranslated message', () => {
        const wrapper = shallow(<Error status={404} messageI18N='not-found' />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });
});

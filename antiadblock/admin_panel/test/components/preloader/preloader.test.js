import React from 'react';
import {shallow} from 'enzyme';
import toJson from 'enzyme-to-json';

import Preloader from 'app/components/preloader/preloader';

describe('preloader component', () => {
    beforeAll(() => {
        console.error = error => { // eslint-disable-line no-console
            throw new Error(error);
        };
    });

    test('without props', () => {
        const wrapper = shallow(<Preloader />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('with fit prop', () => {
        const wrapper = shallow(<Preloader fit='viewport' />);
        expect(toJson(wrapper)).toMatchSnapshot();
    });

    test('with invalid fit prop', () => {
        const wrapperFunction = () => shallow(<Preloader fit='window' />);
        expect(wrapperFunction).toThrowErrorMatchingSnapshot();
    });
});

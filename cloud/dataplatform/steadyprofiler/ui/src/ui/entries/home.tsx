import React from 'react';
import ReactDOM from 'react-dom';
import { ThemeProvider } from '@yandex-data-ui/common';
import '@yandex-data-ui/common/styles/styles.scss';
import Home from '../components/Home/Home';

ReactDOM.render((
    <ThemeProvider theme="dark">
        <Home />
    </ThemeProvider>
), document.getElementById('root'));

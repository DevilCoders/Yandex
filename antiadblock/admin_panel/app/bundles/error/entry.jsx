import bundle from '../initialization';
import App from './error';

const appBundle = bundle(App);

if (module.hot) {
    /*
        https://github.com/gaearon/react-hot-loader/pull/669
        надо сделать через AppContainer warnings: false
        но этот код написан в библиотеке https://github.com/frux/trowel-tools/commit/c1764b78fe31743d8e44261c30ebe2c5870da69f#diff-a06f17dab65ae7085c3487673e18e81dR36
        и у нас нет доступа в нее
     */
    if (process.env.NODE_ENV === 'local') {
        __REACT_HOT_LOADER__.warnings = false; // eslint-disable-line no-undef
    }
    module.hot.accept('./error.jsx', () => {
        const NextApp = require('./error.jsx').default;
        appBundle.updateClient(NextApp);
    });
}

module.exports = appBundle.initClient;

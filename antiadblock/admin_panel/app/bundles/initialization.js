import React from 'react';
import {render} from 'react-dom';
import {AppContainer} from 'react-hot-loader';
import {BrowserRouter} from 'react-router-dom';
import {Provider} from 'react-redux';
import {openConfirmDialog, closeConfirmDialog} from 'app/actions/confirm-dialog';

function getUserConfirmation(store) {
    return (message, callback) => {
        store.dispatch(openConfirmDialog(message, result => {
            store.dispatch(closeConfirmDialog());
            callback(result);
        }));
    };
}

module.exports = function(App) {
    const appBundle = {
        initClient: function (appState, appData, mountElementId) {
            if (appState === undefined) {
                appState = {};
            }
            if (appData === undefined) {
                appData = {};
            }
            if (mountElementId === undefined) {
                mountElementId = 'mount';
            }

            let store = App.configureStore(appState);

            appBundle.updateClient = function (NextApp) {
                const mount = document.getElementById(mountElementId);
                render(
                    <Provider store={store}>
                        <BrowserRouter
                            basename={appData.basename}
                            getUserConfirmation={getUserConfirmation(store)}>
                            <AppContainer>
                                <NextApp appData={appData} />
                            </AppContainer>
                        </BrowserRouter>
                    </Provider>,
                    mount
                );
            };

            appBundle.updateClient(App);
        }
    };

    return appBundle;
};

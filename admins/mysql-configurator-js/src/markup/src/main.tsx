/// <reference types="jquery" />
/// <reference types="flot" />
/// <reference path="socketio.d.ts" />

import { Page } from "./common/page";
import { GraphsPage } from "./graphsPage";

class MainPage extends Page {
    constructor() {
        super();
        this.pageName = "Графики баз";
    }
    getPage(url: string) {
        return <GraphsPage/>;
    }
}

window.onload = function() {
    ReactDOM.render(<MainPage/>, document.getElementById("data"));
};

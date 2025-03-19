/// <reference types="jquery" />

import { HostPage } from "./pages/host";
import { Distribution } from "./pages/distribution";
import { Demo } from "./pages/demo";
import { Resources } from "./pages/resources";
import { Page } from "./common/page";

class MainPage extends Page {
    constructor() {
        super();
        this.pageName = "Мороженка";
        this.menu = [
            { url: "/", name: "Распределение",
              page: <Distribution pushState={this.pushState.bind(this)} /> },
            { url: "/resources", name: "Ресурсы", page: <Resources /> }
    //        { url: "/demo", name: "Демо", page: <Demo/> }
        ];
    }

    getPage(path: string): JSX.Element {

        let items = path.split("/");
        switch (items[1]) {
            case "host":
                return <HostPage fqdn={items[2]} />;
        }
    }
}

$(function () {
    ReactDOM.render(<MainPage />, document.getElementById("data"));
});

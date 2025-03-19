import { combineEpics, createEpicMiddleware } from 'redux-observable';
import shipmentEpics from "./shipmentEpics";
import shipmentsEpics from "./shipmentsEpics";
import mastersEpics from "./mastersEpics";
import headerEpics from "./headerEpics";
import {Api} from "../models/deployapi";
import minionsEpics from "./minionsEpics";

const deployAPI = new Api({baseUrl:'http://localhost:3001'});

export const rootEpic = combineEpics<any>(
    mastersEpics,
    shipmentEpics,
    shipmentsEpics,
    headerEpics,
    minionsEpics
);

export default createEpicMiddleware({
    dependencies: {deployAPI: deployAPI}
});

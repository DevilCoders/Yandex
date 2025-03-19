// подключаем модуль для работы с датами
const moment = require('vendor/moment/v2.21');

// подключаем модуль источника данных
const ClickHouse = require('libs/chyt/v1');

// запрашиваем загруженные данные
const loadedData = ChartEditor.getLoadedData();

// достаем поля из загруженных данных
const values = loadedData.clickHouseSource.data;
const params = ChartEditor.getParams();
var fields = params.billing_account_id;


const categories_ms = new Set();

const account_names_map = {};


const datas = values.reduce((datas, value) => {
    fields.forEach((field) => {
        if ((value.billing_account_id == field) | (field =='All')){
            datas[value.billing_account_id] = datas[value.billing_account_id] || [];
            datas[value.billing_account_id].push({'x': moment.utc(value.time).valueOf(),
                               'y': value.sub_count});
        }
    });
    account_names_map[value.billing_account_id] = value.name
    return datas;
}, {});


const graphs = Object.keys(datas).reduce((graphs, key) => {
    graphs.push({
        id: key,
        name: account_names_map[key],
        data: datas[key]
    });
    return graphs;
}, []);




module.exports = {graphs:graphs};


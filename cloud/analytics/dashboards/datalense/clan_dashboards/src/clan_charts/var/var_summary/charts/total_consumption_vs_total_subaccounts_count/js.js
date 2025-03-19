// подключаем модуль для работы с датами
const moment = require('vendor/moment/v2.21');

// подключаем модуль источника данных
const ClickHouse = require('libs/chyt/v1');

// запрашиваем загруженные данные
const loadedData = ChartEditor.getLoadedData();

// достаем поля из загруженных данных
const values = loadedData.clickHouseSource.data;
const params = ChartEditor.getParams();
var fields = ['total'];



const categories_ms = new Set();


values.forEach((value) => {
    categories_ms.add( moment.utc(value.time).valueOf());
});

const categories = Array.from(categories_ms).sort()


const sub_count = values.reduce((sub_count, value) => {
    fields.forEach((field) => {
        if ((value.billing_account_id == field) | (field =='All')){
            sub_count[value.billing_account_id] = sub_count[value.billing_account_id] || [];
            sub_count[value.billing_account_id].push(value.total_sub_count);
        }
    });
    return sub_count;
}, {});


const sub_cons = values.reduce((sub_cons, value) => {
    fields.forEach((field) => {
        if ((value.billing_account_id == field) | (field =='All')){
            sub_cons[value.billing_account_id] = sub_cons[value.billing_account_id] || [];
            sub_cons[value.billing_account_id].push( value.total_sub_consumption);
        }
    });
    return sub_cons;
}, {});



const graphs_count = Object.keys(sub_count).reduce((graphs_count, key) => {
    graphs_count.push({
        yAxis: 1,
        name: 'total_sub_count',
        data: sub_count[key]
    });
    return graphs_count;
}, []);


const graphs_cons = Object.keys(sub_cons).reduce((graphs_cons, key) => {
    graphs_cons.push({
        yAxis : 0,
        name: 'total_sub_cons',
        data: sub_cons[key]
    });
    return graphs_cons;
}, []);

const graphs = [graphs_count[0], graphs_cons[0]];
console.log(graphs)
module.exports = {graphs:graphs, categories_ms:categories};
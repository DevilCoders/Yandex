def url(query):
    return  '''
const ClickHouse = require('libs/chyt/v1');
const moment = require('vendor/moment/v2.21');

const params = ChartEditor.getParams();
const {from, to} = ChartEditor.resolveInterval(params.from);
const conv_d = ChartEditor.resolveInterval(params.converted_date);
const from_conv_m = moment(conv_d.from).valueOf()/1000;
const to_conv_m = moment(conv_d.to).valueOf()/1000; 
''' + query + '''
console.log(query);
// формируем источник данных
const clickHouseSource = ClickHouse.buildSource({
    cluster: 'hahn',
    query: query,
    cliqueId:'*cloud_analytics_datalens',
    token: ChartEditor.getSecrets()['yt_token'], // yt token
    cache: 360
});

// экспортируем источник данных
module.exports = {
    clickHouseSource: clickHouseSource
};
'''
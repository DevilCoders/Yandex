{%- set log_settings = pillar['oct']['log_settings']['contrail-web'] -%}
/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

var config = {};

config.orchestration = {};
config.orchestration.Manager = 'none';
config.staticAuth = [
      {username: 'admin', password: 'contrail123', roles: ['cloudAdmin']}
];

/****************************************************************************
 * This boolean flag indicates to communicate with Orchestration
 * modules(networkManager, imageManager, computeManager, identityManager,
 * storageManager), should the webServer communicate using the
 * ip/port/authProtocol/apiVersion as specified in this file, or as returned
 * from auth catalog list.
 * Note: config.identityManager.apiVersion is not controlled by this boolean
 * flag.
 *
 * true  - These values should be taken from this config
 *         file.
 * false - These values should be taken from auth catalog list 
 *
*****************************************************************************/
config.serviceEndPointFromConfig = true;


/****************************************************************************
 * This boolean flag specifies wheather region list should be taken from config
 * file or from keystone endpoint
 * true  - If set as true, then keystone endpoint is taken from
 *         config.regions
 * false - If set as false, then keystone endpoint is taken from
 *         config.identityManager
 *
 ****************************************************************************/
config.regionsFromConfig = true;

/****************************************************************************
 * Below are the configs for Api Server and analytics Service type & name as
 * provisioned in keystone
 *
 * apiServiceType - Service Type for apiServer, default value is apiServer
 * opServiceType  - Service Type for analytics, default value is opServer
 *
 * Note: If there are multiple api server or analytices nodes in a specific
 *       region, then provision service type/name as ApiServer0, ApiServer1,
 *       ApiServer2 etc, similarly for analytics node: OpServer0, OpServer1,
 *       OpServer2 etc.
 *
 ****************************************************************************/
config.endpoints = {};
config.endpoints.apiServiceType = 'ApiServer';
config.endpoints.opServiceType = 'OpServer';

/****************************************************************************
 * Mapping to region name with keystone endpoint
 *
 * For example:
 * config.regions.RegionOne = 'http://nodeIp:5000/v2.0';
 * config.regions.RegionTwo = 'http://nodeIp:5000/v3';
 *
 ****************************************************************************/
config.regions = {};
config.regions.RegionOne = 'http://127.0.0.1:5000/v2.0';

/****************************************************************************
 * This boolean flag indicates if serviceEndPointFromConfig is set as false,
 * then to take IP/Port/Protocol/Version information from auth catalog, 
 * should publicURL OR internalURL will be used.
 *
 * true  - publicURL in endpoint will be used to retrieve IP/Port/Protocol/
 *         Version information
 * false - internalURL in endpoint will be used to retrieve
 *         IP/Port/Protocol/Version information
 *
 * NOTE: if config.serviceEndPointFromConfig is set as true, then this flag
 *       does not have any effect.
 *
*****************************************************************************/
config.serviceEndPointTakePublicURL = true;

/****************************************************************************
 * Below are the config options for all Orchestration Modules below:
 *  - networkManager
 *  - imageManager
 *  - computeManager
 *  - identityManager
 *  - storageManager
 *  - cnfg
 *  - analytics
 *
 * Options:
 * ip:
 *      IP to connect to for this Server.
 * port:
 *      Port to connect to for this server
 * authProtocol:        
 *      Specify authProtocol either 'http' or 'https'
 * apiVersion:
 *      REST API Version for this server to connect to.
 *      Specify a list of Versions in array notation.
 *      Below are the supported list of apiVersion for the modules as of now:
 *      imageManager    -   ['v1', 'v2']
 *      computeManager  -   ['v1.1', 'v2']
 *      identityManager -   ['v2.0']
 *      storageManager  -   ['v1']
 *
 *      Not applicable for cnfg/analytics as of now
 * strictSSL:
 *      If true, requires certificates to be valid
 * ca: 
 *      An authority certificate to check the remote host against,
 *      if you do not want to specify then use ''
*****************************************************************************/
config.networkManager = {};
config.networkManager.ip = '127.0.0.1';
config.networkManager.port = '9696'
config.networkManager.authProtocol = 'http';
config.networkManager.apiVersion = [];
config.networkManager.strictSSL = false;
config.networkManager.ca = '';

config.imageManager = {};
config.imageManager.ip = '127.0.0.1';
config.imageManager.port = '9292';
config.imageManager.authProtocol = 'http';
config.imageManager.apiVersion = ['v1', 'v2'];
config.imageManager.strictSSL = false;
config.imageManager.ca = '';

config.computeManager = {};
config.computeManager.ip = '127.0.0.1';
config.computeManager.port = '8774';
config.computeManager.authProtocol = 'http';
config.computeManager.apiVersion = ['v1.1', 'v2'];
config.computeManager.strictSSL = false;
config.computeManager.ca = '';

config.identityManager = {};
config.identityManager.ip = '127.0.0.1';
config.identityManager.port = '5000';
config.identityManager.authProtocol = 'http';
/******************************************************************************
 * Note: config.identityManager.apiVersion is not controlled by boolean flag 
 * config.serviceEndPointFromConfig. If specified apiVersion here, then these
 * API versions will be used while using REST API to identityManager.
 * If want to use with default apiVersion(v2.0), then can specify it as 
 * empty array.
******************************************************************************/
config.identityManager.apiVersion = ['v2.0'];
config.identityManager.strictSSL = false;
config.identityManager.ca = '';

config.storageManager = {};
config.storageManager.ip = '127.0.0.1';
config.storageManager.port = '8776';
config.storageManager.authProtocol = 'http';
config.storageManager.apiVersion = ['v1'];
config.storageManager.strictSSL = false;
config.storageManager.ca = '';

// VNConfig API server and port.
config.cnfg = {};
config.cnfg.server_ip = '{{ ipv4 }}';
config.cnfg.server_port = '8082';
config.cnfg.authProtocol = 'http';
config.cnfg.strictSSL = false;
config.cnfg.ca = '';

// Analytics API server and port.
config.analytics = {};
config.analytics.server_ip = '{{ analytics_ipv4 }}';
config.analytics.server_port = '8081';
config.analytics.authProtocol = 'http';
config.analytics.strictSSL = false;
config.analytics.ca = '';

// vcenter related parameters
config.vcenter = {};
config.vcenter.server_ip = '127.0.0.1';         //vCenter IP
config.vcenter.server_port = '443';             //Port
config.vcenter.authProtocol = 'https';          //http or https
config.vcenter.datacenter = 'vcenter';          //datacenter name
config.vcenter.dvsswitch = 'vswitch';           //dvsswitch name
config.vcenter.strictSSL = false;               //Validate the certificate or ignore
config.vcenter.ca = '';                         //specify the certificate key file
config.vcenter.wsdl = '/var/lib/contrail-webui/contrail-web-core/webroot/js/vim.wsdl';

/* Discovery Service */
config.discoveryService = {};
config.discoveryService.server_ip = '127.0.0.1';
config.discoveryService.server_port = '5998';
/* Specifiy true if subscription to discovery server should be enabled, else
 * specify false. Other than true/false value here is treated as true
 */
config.discoveryService.enable = true;

/* Job Server */
config.jobServer = {};
config.jobServer.server_ip = '127.0.0.1';
config.jobServer.server_port = '3000';

/* Upload/Download Directory */
config.files = {};
config.files.download_path = '/tmp';

/* Cassandra Server */
config.cassandra = {};
config.cassandra.server_ips = [{%- for server in oct_db_servers %}'{{ grains['cluster_map']['hosts'][server]['ipv4']['addr'] }}'{% if not loop.last %},{% endif %}{%- endfor %}];
config.cassandra.server_port = '9042';
config.cassandra.enable_edit = false;

/* KUE Job Scheduler */
config.kue = {};
config.kue.ui_port = '3002';

/* IP List to listen on */
config.webui_addresses = ['0.0.0.0'];

/* Is insecure access to WebUI? 
 * If set as false, then all http request will be redirected
 * to https, if set true, then no https request will be processed, but only http
 * request
 */
config.insecure_access = true;

// HTTP port for NodeJS Server.
config.http_port = '8888';

// HTTPS port for NodeJS Server.
config.https_port = '8143';

// Activate/Deactivate Login.
config.require_auth = false;

/* Number of node worker processes for cluster. */
config.node_worker_count = 1;

/* Number of Parallel Active Jobs with same type */
config.maxActiveJobs = 10;

/* Redis DB index for Web-UI */
config.redisDBIndex = 3;

/* WebUI Redis Server */
config.redis_server_port = '6379';
config.redis_server_ip = '127.0.0.1';
config.redis_dump_file = '/var/lib/redis/dump-webui.rdb';
config.redis_password = '';

/* Cache Expiry Time */
config.cacheExpire = {};
config.cacheExpire.flow_stat_time = 600; /* Seconds */
config.cacheExpire.topo_tree_time = 600; /* Seconds */

/* Logo File: Use complete path of logo file location */
config.logo_file = '/var/lib/contrail-webui/contrail-web-core/webroot/img/opencontrail-logo.png';

/* Favicon File: Use complete path of favicon file location */
config.favicon_file = '/var/lib/contrail-webui/contrail-web-core/webroot/img/opencontrail-favicon.ico';

config.featurePkg = {};
/* Add new feature Package Config details below */
config.featurePkg.webController = {};
config.featurePkg.webController.path = '/var/lib/contrail-webui/contrail-web-controller';
config.featurePkg.webController.enable = true;

/* Enable/disable Stat Query Links in Sidebar*/
config.qe = {};
config.qe.enable_stat_queries = false;

/* Configure level of logs, supported log levels are:
   debug, info, notice, warning, error, crit, alert, emerg
 */
config.logs = {};
config.logs.level = '{{ log_settings['level'] }}';

/******************************************************************************
 * Boolean flag getDomainProjectsFromApiServer indicates wheather the project
 * list should come from API Server or Identity Manager.
 * If Set
 *      - true, then project list will come from API Server
 *      - false, then project list will come from Identity Manager
 * Default: false
 *
 ******************************************************************************/
config.getDomainProjectsFromApiServer = false;
/*****************************************************************************
 * Boolean flag L2_enable indicates the default forwarding-mode of a network.
 * Allowed values : true / false
 * Set this flag to true if all the networks are to be L2 networks,
 * set to false otherwise.
 *****************************************************************************/
config.network = {};
config.network.L2_enable = false;

/******************************************************************************
 * Boolean flag getDomainsFromApiServer indicates wheather the domain
 * list should come from API Server or Identity Manager.
 * If Set
 *      - true, then domain list will come from API Server
 *      - false, then domain list will come from Identity Manager
 * Default: true
 * NOTE: if config.identityManager.apiVersion is set as v2.0, then this flag
 *       does not have any effect, in that case the domain list is retrieved
 *       from API Server.
 *
 *****************************************************************************/
config.getDomainsFromApiServer = true;

config.jsonSchemaPath = "/var/lib/contrail-webui/contrail-web-core/src/serverroot/configJsonSchemas";

// Export this as a module.
module.exports = config;

// This file overrides the Tez UI default config when using the Data Proc UI Proxy
// Sets the target host and base path for API requests from client JS
ENV = {
  hosts: {
    timeline: "{{ cluster_ui_hostname }}",
    rm: "{{ cluster_ui_hostname }}",
  },

  namespaces: {
    webService: {
      timeline: "gateway/default-topology/apphistory/ws/v1/timeline",
      appHistory: "gateway/default-topology/apphistory/ws/v1/applicationhistory",
      rm: "gateway/default-topology/yarn/ws/v1/cluster",
      am: "gateway/default-topology/yarn/proxy/{app_id}/ws/v{version:2}/tez",
    },
  },
};

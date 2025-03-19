// This file overrides the Tez UI default config when using the Data Proc UI Proxy
// Sets the target host and base path for API requests from client JS
ENV = {
  hosts: {
    timeline: "{{ timeline }}",
    rm: "{{ resourcemanager }}",
  }
};

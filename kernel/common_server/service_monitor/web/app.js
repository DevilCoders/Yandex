angular
  .module("service_monitor", ["ngRoute"])
  .controller("mainController", function ($scope, $http, $routeParams) {
    function update_service_info() {
      $http
        .get("/service_info", {
          params: { service: $routeParams.service, dump: $routeParams.dump },
        })
        .then(
          function onSuccess(response) {
            $scope.services = response.data.result.services;
          },
          function onError(response) {
            console.log(response);
          }
        );
    }
    update_service_info();
    if ($routeParams.refresh_ms > 0) {
      setInterval(update_service_info, $routeParams.refresh_ms);
    }

    $scope.round = function (value) {
      return Math.round(value * 100) / 100;
    };
    $scope.controller_command = function (host, command, params = {}) {
        params['pod_host'] = host;
        params['command'] = command;
        params['async'] = 'yes';
        $http
            .get("/controller_command", {
                params: params
            })
            .then(
                function onSuccess(response) {
                },
                function onError(error) {
                    alert(error.status + ":" + JSON.stringify(error.data));
                }
            );
    }
  })
  .config(function ($routeProvider) {
    $routeProvider.when("/", {
      templateUrl: "main.html",
      controller: "mainController",
    });
  });

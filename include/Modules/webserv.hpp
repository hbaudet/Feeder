#pragma once

#include <string>

#include "esp_log.h"
#include "esp_http_server.h"

#include "Modules/outputManager.hpp"
#include "Modules/routine.hpp"
#include "helpers.hpp"

#define WEBSERV_PORT 80

class WebServ {
    public:
        WebServ(const WebServ&) = delete;
        WebServ         &operator=(const WebServ&) = delete;
        static void     run();
        static void     setHooks(Routine *, OutputManager *);

    private:
        static WebServ      &instance();
        static esp_err_t    get_log_handler(httpd_req_t *req);
        static esp_err_t    index_get_handler(httpd_req_t *req);
        static esp_err_t    api_status_handler(httpd_req_t *req);
        // TODO : API hooks
        // static esp_err_t    routine_status_handler(httpd_req_t *req); // get schedule
        // static esp_err_t    routine_update_handler(httpd_req_t *req);
        // static esp_err_t    output_handler(httpd_req_t *req);

        WebServ();

        Routine             *routine;
        OutputManager       *output;
        httpd_handle_t      server;
        httpd_config_t      config;
};

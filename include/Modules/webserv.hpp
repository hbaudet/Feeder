#pragma once

#include <string>

#include "esp_log.h"
#include "esp_http_server.h"
#include <esp_timer.h>

#include "Modules/outputManager.hpp"
#include "Modules/routine.hpp"
#include "helpers.hpp"
#include "hmi.hpp"

#define WEBSERV_PORT        80
#define API_ADD_BASE_URI    "/api/add/"

class WebServ {
    public:
        WebServ(const WebServ&) = delete;
        WebServ         &operator=(const WebServ&) = delete;
        static void     run();
        static void     setHooks(Routine *, OutputManager *, HMI *);

    private:
        static WebServ      &instance();
        static esp_err_t    favicon(httpd_req_t *req);
        static esp_err_t    getLogHandler(httpd_req_t *req);
        static esp_err_t    getIndexHandler(httpd_req_t *req);
        static esp_err_t    rebootHandler(httpd_req_t *req);
        static esp_err_t    lockHandler(httpd_req_t *req);
        static esp_err_t    unlockHandler(httpd_req_t *req);
        static esp_err_t    apiStatusHandler(httpd_req_t *req);
        static esp_err_t    apiScheduleHandler(httpd_req_t *req);
        static esp_err_t    apiEventAddHandler(httpd_req_t *req);
        static esp_err_t    apiUpdateScheduleHandler(httpd_req_t *req);

        static void         getUptimeJson(JsonDocument &);
        static void         getTimeJson(JsonDocument &);
        static void         getNextEventJson(JsonDocument &);
        static void         getModulesStatusJson(JsonDocument &);

        WebServ();

        Routine             *routine;
        OutputManager       *output;
        HMI                 *hmi;
        httpd_handle_t      server;
        httpd_config_t      config;
};

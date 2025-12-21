#include "Modules/webserv.hpp"

static const char *TAG = "webserv";

WebServ::WebServ() : server(nullptr) {
    config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEBSERV_PORT;
    config.uri_match_fn = httpd_uri_match_wildcard;
}

WebServ     &WebServ::instance() {
    static WebServ instance;
    return instance;
}

esp_err_t   WebServ::get_log_handler(httpd_req_t *req) {
    // TODO : see if websocket for live logging doable
    httpd_resp_set_type(req, "text/html");
    std::string logs = getLogs();
    httpd_resp_send(req, logs.c_str(), logs.size());
    return ESP_OK;
}

esp_err_t   WebServ::index_get_handler(httpd_req_t *req) {
    // might be deprecated if no use for it is found
    ESP_LOGI(TAG, "GET /");
    auto html = readFile("/spiffs/index.html");
    if (html.empty()) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.size());
    return ESP_OK;
}

esp_err_t   WebServ::api_status_handler(httpd_req_t *req) {
    // TODO : include modules status
    // TODO : get uptime
    // TODO : get hardware info if available (mem usage, temp, etc..)
    // TODO : get network info (whatever is available)
    // TODO : get current time
    // TODO : get last event
    // TODO : get next event
    std::string resp = R"({"status":"ok","uptime":1234})";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp.c_str(), resp.size());
    return ESP_OK;
}

void        WebServ::setHooks(Routine *rout, OutputManager *out) {
    instance().routine = rout;
    instance().output = out;
}

void        WebServ::run() {
    if (httpd_start(&(instance().server), &(instance().config)) == ESP_OK)
    {
        logToWebInit();
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_get_handler,
            .user_ctx = nullptr};

        httpd_uri_t status_uri = {
            .uri = "/api/status",
            .method = HTTP_GET,
            .handler = api_status_handler,
            .user_ctx = nullptr};

        httpd_uri_t logs_uri = {
            .uri = "/logs",
            .method = HTTP_GET,
            .handler = get_log_handler,
            .user_ctx = nullptr};

        httpd_register_uri_handler((instance().server), &index_uri);
        httpd_register_uri_handler((instance().server), &status_uri);
        httpd_register_uri_handler((instance().server), &logs_uri);

        ESP_LOGI(TAG, "HTTP server ready on port %d", instance().config.server_port);
    }
}

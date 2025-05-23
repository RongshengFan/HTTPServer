#pragma once
#include "../../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"

class LogoutHandler : public http::router::RouterHandler 
{
public:
    explicit LogoutHandler(GomokuServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
private:
    GomokuServer* server_;
};
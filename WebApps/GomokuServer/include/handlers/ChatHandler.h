#pragma once

#include "../../../HttpServer/include/router/RouterHandler.h"
#include "../GomokuServer.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ChatHandler : public http::router::RouterHandler
{
public:
    explicit ChatHandler(GomokuServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;
    
private:
    // 处理聊天页面请求
    void handleChatPage(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // 处理聊天完成请求
    void handleChatCompletion(const http::HttpRequest& req, http::HttpResponse* resp);
    
    // 调用火山方舟API
    std::string callVolcanoArkAPI(const std::string& userMessage);
    
    GomokuServer* server_;
    const std::string API_KEY = "6b65c93e-5501-436e-b34f-991d5fbe3c6b";
    const std::string MODEL_NAME = "doubao-seed-1-6-flash-250715";
    const std::string API_URL = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";
};
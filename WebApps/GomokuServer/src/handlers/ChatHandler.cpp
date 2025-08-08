#include "../include/handlers/ChatHandler.h"
#include "../../../HttpServer/include/http/HttpRequest.h"
#include "../../../HttpServer/include/http/HttpResponse.h"
#include "../../../HttpServer/include/utils/FileUtil.h"
#include "../../../HttpServer/include/utils/JsonUtil.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

using namespace http;
using json = nlohmann::json;

// 回调函数用于接收API响应
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch (std::bad_alloc& e) {
        // 处理内存分配失败
        return 0;
    }
    return newLength;
}

void ChatHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp) {
    try {
        if (req.method() == http::HttpRequest::kGet) {
            handleChatPage(req, resp);
        } else if (req.method() == http::HttpRequest::kPost) {
            handleChatCompletion(req, resp);
        } else {
            json errorResp;
            errorResp["status"] = "error";
            errorResp["message"] = "Method Not Allowed";
            std::string errorBody = errorResp.dump(4);

            // 修正：明确使用http::HttpResponse
            server_->packageResp(req.getVersion(), http::HttpResponse::k405MethodNotAllowed,
                                "Method Not Allowed", true, "application/json", 
                                errorBody.size(), errorBody, resp);
        }
    } catch (const std::exception& e) {
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = e.what();
        std::string errorBody = errorResp.dump(4);

        // 修正：明确使用http::HttpResponse
        server_->packageResp(req.getVersion(), http::HttpResponse::k500InternalServerError,
                            "Internal Server Error", true, "application/json",
                            errorBody.size(), errorBody, resp);
    }
}

void ChatHandler::handleChatPage(const http::HttpRequest& req, http::HttpResponse* resp) {
    // 检查用户是否登录（现在可访问getSessionManager()，因已声明友元）
    auto session = server_->getSessionManager()->getSession(req, resp);
    LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
    if (session->getValue("isLoggedIn") != "true") {
        // 用户未登录，重定向到登录页面
        // 修正：明确使用http::HttpResponse
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k301MovedPermanently, "Moved Permanently");
        resp->addHeader("Location", "/entry");
        resp->setCloseConnection(true);
        return;
    }
    
    // 获取用户ID
    int userId = std::stoi(session->getValue("userId"));
    
    // 读取聊天页面
    std::string reqFile("../WebApps/GomokuServer/resource/Chat.html");
    FileUtil fileOperater(reqFile);
    if (!fileOperater.isValid()) {
        LOG_WARN << reqFile << " not exist.";
        fileOperater.resetDefaultFile();
    }

    std::vector<char> buffer(fileOperater.size());
    fileOperater.readFile(buffer);
    std::string chatHtml(buffer.data(), buffer.size());

    // 在HTML中插入userId
    size_t headEnd = chatHtml.find("</head>");
    if (headEnd != std::string::npos) {
        std::string script = "<script>const userId = '" + std::to_string(userId) + "';</script>";
        chatHtml.insert(headEnd, script);
    }

    // 封装响应
    resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
    resp->setCloseConnection(false);
    resp->setContentType("text/html; charset=utf-8");
    resp->setContentLength(chatHtml.size());
    resp->setBody(chatHtml);
}

void ChatHandler::handleChatCompletion(const http::HttpRequest& req, http::HttpResponse* resp) {
    // 检查用户是否登录（现在可访问getSessionManager()，因已声明友元）
    auto session = server_->getSessionManager()->getSession(req, resp);
    LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
    if (session->getValue("isLoggedIn") != "true") {
        json errorResp;
        errorResp["status"] = "error";
        errorResp["message"] = "Please login first";
        std::string errorBody = errorResp.dump(4);

        // 修正：明确使用http::HttpResponse
        server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
                            "Unauthorized", true, "application/json",
                            errorBody.size(), errorBody, resp);
        return;
    }
    
    try {
        // 解析请求体
        json requestBody = json::parse(req.getBody());
        std::string userMessage = requestBody["message"];
        
        // 调用火山方舟API
        std::string apiResponse = callVolcanoArkAPI(userMessage);
        
        // 设置响应（现在可访问packageResp()，因已声明友元）
        server_->packageResp(req.getVersion(), http::HttpResponse::k200Ok, "OK",
                            false, "application/json", apiResponse.size(),
                            apiResponse, resp);
    } catch (const std::exception& e) {
        json errorResp;
        errorResp["error"] = "Internal Error";
        errorResp["message"] = e.what();
        std::string errorBody = errorResp.dump();
        
        // 错误响应设置
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error");
        resp->setCloseConnection(true);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
    }
}

std::string ChatHandler::callVolcanoArkAPI(const std::string& userMessage) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }
    
    // 构建请求JSON
    json requestJson;
    requestJson["model"] = MODEL_NAME;  // 确保MODEL_NAME已定义
    
    json messages = json::array();
    messages.push_back({{"role", "system"}, {"content", "You are a helpful assistant. Do not use deep thinking."}});
    messages.push_back({{"role", "user"}, {"content", userMessage}});
    
    requestJson["messages"] = messages;
    std::string requestBody = requestJson.dump();
    
    // 设置CURL选项
    std::string responseString;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    std::string authHeader = "Authorization: Bearer " + API_KEY;  // 确保API_KEY已定义
    headers = curl_slist_append(headers, authHeader.c_str());
    
    curl_easy_setopt(curl, CURLOPT_URL, API_URL.c_str());  // 确保API_URL已定义
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
    
    // 执行请求
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string errorMsg = "API request failed: " + std::string(curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        LOG_ERROR << errorMsg;
        throw std::runtime_error(errorMsg);
    }
    
    // 清理资源
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    LOG_INFO << "Volcano Ark API response: " << responseString;
    
    return responseString;
}
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handle_CGI.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gabrsouz <gabrsouz@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 12:57:41 by kmaeda            #+#    #+#             */
/*   Updated: 2026/02/12 12:38:03 by gabrsouz         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/http/Response.hpp"
#include "../../include/cgi/CGI.hpp"
#include "../../include/http/Request.hpp"

void Response::parseHeaderLines(const std::string headers_part) {
    // parse header lines
    std::istringstream hs(headers_part);
    std::string line;
    bool status_set = false;
    while (std::getline(hs, line)) {
        // strip \r if present (avoid using string::back/pop_back for C++98)
        if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1, 1);
        if (line.empty()) continue;
        size_t pos = line.find(":" );
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        // trim leading spaces
        size_t start = value.find_first_not_of(" \t");
        if (start != std::string::npos) value = value.substr(start);
        // handle Status header specially
        if (key == "Status") {
            std::istringstream iss(value);
            int code = 200;
            iss >> code;
            this->status = code;
            status_set = true;
        } else {
            headers[key] = value;
        }
    }
    if (!status_set) this->status = 200;
}

std::string findFullPath(const std::string& rootDir, const std::string path) {
    std::string newPath = rootDir;
    if (!newPath.empty() && newPath[newPath.size() - 1] != '/')
        newPath += '/';
    size_t qpos = path.find('?');
    std::string path_no_query = (qpos != std::string::npos) ? path.substr(0, qpos) : path;
    if (!path_no_query.empty() && path_no_query[0] == '/')
        newPath += path_no_query.substr(1);
    else
        newPath += path_no_query;
    return newPath;
}

// parse raw into headers and body
void separateHeadersBody(std::string raw, std::string& headers_part, std::string& body_part) {
    size_t sep = raw.find("\r\n\r\n");
    size_t seplen = 4;
    if (sep == std::string::npos) {
        sep = raw.find("\n\n");
        seplen = 2;
    }
    if (sep != std::string::npos) {
        headers_part = raw.substr(0, sep);
        body_part = raw.substr(sep + seplen);
    } else
        body_part = raw;
}


// Handle CGI requests: execute CGI and parse its output
std::string Response::handleCgi(const Request& request, const ServerConfig& serverCfg, const std::string& rootDir, const std::string& cgiPath) {
    CGI cgi;

    // Basic validation: CGI interpreter must be configured
    if (cgiPath.empty())
        return errorResponse(500, "CGI interpreter not configured");
    const std::string path = request.getPath();
    if (!isSafePath(path))
        return errorResponse(403, "Forbidden");
    // Build script full path (strip query part)
    std::string scriptPath = findFullPath(rootDir, path);
    if (!fileExists(scriptPath))
        return errorResponse(404, "Not Found");
    // Execute CGI — returns raw output (headers + body)
    std::string raw;
    try {
        raw = cgi.execute(request, serverCfg, scriptPath, cgiPath);
    } catch (...) {
        return errorResponse(500, "CGI execution failed");
    }
    if (raw.compare(0, 5, "HTTP/") == 0)
        return raw;
    std::string headers_part, body_part;
    separateHeadersBody(raw, headers_part, body_part);
    parseHeaderLines(headers_part);

    this->body = body_part;
    if (headers.find("Content-Type") == headers.end())
        headers["Content-Type"] = "text/plain";

    return buildHttpResponse();
}
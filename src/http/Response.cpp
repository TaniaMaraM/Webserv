/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmaeda <kmaeda@student.42berlin.de>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 12:57:41 by kmaeda            #+#    #+#             */
/*   Updated: 2026/03/17 15:18:36 by kmaeda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/http/Response.hpp"
#include "../../include/cgi/CGI.hpp"
#include "../../include/http/Request.hpp"

Response::Response() {}

// Store error page configuration from server config
void Response::setErrorPages(const std::map<int, std::string>& pages, const std::string& root) {
	errorPages = pages;
	errorRoot = root;
}

// Get custom error page path for a status code, or empty string if not configured
std::string Response::getErrorPagePath(int statusCode) {
	if (errorPages.find(statusCode) != errorPages.end())
		return errorRoot + "/" + errorPages[statusCode];
	return "";
}

// Build complete HTTP response with status line, headers, and body
std::string Response::buildHttpResponse(){
	std::string response;
    
    switch (status) {
        case 200:
            response += "HTTP/1.1 200 OK\r\n";
            break;
        case 201:
            response += "HTTP/1.1 201 Created\r\n";
            break;
        case 301:
            response += "HTTP/1.1 301 Moved Permanently\r\n";
            break;
        case 302:
            response += "HTTP/1.1 302 Found\r\n";
            break;
        case 307:
            response += "HTTP/1.1 307 Temporary Redirect\r\n";
            break;
        case 308:
            response += "HTTP/1.1 308 Permanent Redirect\r\n";
            break;
        case 400:
            response += "HTTP/1.1 400 Bad Request\r\n";
            break;
        case 403:
            response += "HTTP/1.1 403 Forbidden\r\n";
            break;
        case 404:
            response += "HTTP/1.1 404 Not Found\r\n";
            break;
        case 405:
            response += "HTTP/1.1 405 Method Not Allowed\r\n";
            break;
        case 413:
            response += "HTTP/1.1 413 Request Entity Too Large\r\n";
            break;
        case 500:
            response += "HTTP/1.1 500 Internal Server Error\r\n";
            break;
        case 501:
            response += "HTTP/1.1 501 Not Implemented\r\n";
            break;
        case 505:
            response += "HTTP/1.1 505 HTTP Version Not Supported\r\n";
            break;
        case 504:
            response += "HTTP/1.1 504 Gateway Timeout\r\n";
            break;
        default:
            response += "HTTP/1.1 500 Internal Server Error\r\n";
            break;
    }
    // Add all headers dynamically
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        response += it->first + ": " + it->second + "\r\n";
    }
    std::ostringstream oss;
    oss << body.size();
    response += "Content-Length: " + oss.str() + "\r\n";
    response += "\r\n";
    response += body;
    
    return response;
}

// Determine type based on file extension
std::string Response::getContentType(const std::string& path) {
	// Text types
	if (path.find(".html") != std::string::npos)
        return "text/html";
    if (path.find(".htm") != std::string::npos)
        return "text/html";
    if (path.find(".css") != std::string::npos)
        return "text/css";
    if (path.find(".json") != std::string::npos)
        return "application/json";
    if (path.find(".js") != std::string::npos)
        return "application/javascript";
    if (path.find(".xml") != std::string::npos)
        return "application/xml";
    if (path.find(".txt") != std::string::npos)
        return "text/plain";
    
    // Images
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    if (path.find(".png") != std::string::npos)
        return "image/png";
    if (path.find(".gif") != std::string::npos)
        return "image/gif";
    if (path.find(".svg") != std::string::npos)
        return "image/svg+xml";
    if (path.find(".ico") != std::string::npos)
        return "image/x-icon";
    if (path.find(".webp") != std::string::npos)
        return "image/webp";
    
    // Fonts
    if (path.find(".woff") != std::string::npos)
        return "font/woff";
    if (path.find(".woff2") != std::string::npos)
        return "font/woff2";
    if (path.find(".ttf") != std::string::npos)
        return "font/ttf";
    if (path.find(".otf") != std::string::npos)
        return "font/otf";
    
    // Documents
    if (path.find(".pdf") != std::string::npos)
        return "application/pdf";
    if (path.find(".zip") != std::string::npos)
        return "application/zip";
    
    return "application/octet-stream"; //default
}

// Read entire file contents into string
bool Response::readFile(const std::string& filepath, std::string& contentFile) {
	std::ifstream file(filepath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open())
		return false;
	std::ostringstream ss;
    ss << file.rdbuf(); // read entire file into stringstream
    contentFile = ss.str();
	return true;
}

// Check if file or directory exists
bool Response::fileExists(const std::string& filepath) {
	struct stat buffer;
	return (stat(filepath.c_str(), &buffer) == 0);
}

// Check if path is a directory
bool Response::isDirectory(const std::string& path) {
	struct stat buffer;
	if (stat(path.c_str(), &buffer) != 0)
		return false;
	return S_ISDIR(buffer.st_mode);
}

// Validate path to prevent directory traversal attacks
bool Response::isSafePath(const std::string& path) {
	// Reject paths containing ".."
	if (path.find("..") != std::string::npos)
		return false;
	// Reject paths that don't start with / (invalid HTTP paths)
	if (!path.empty() && path[0] != '/')
		return false;
	// This prevents access to /etc, /usr, /bin, /proc, etc.
	if (path.find("/etc/") == 0 || path.find("/usr/") == 0 || 
	    path.find("/bin/") == 0 || path.find("/sbin/") == 0 ||
	    path.find("/proc/") == 0 || path.find("/sys/") == 0 ||
	    path.find("/dev/") == 0 || path.find("/root/") == 0 ||
	    path.find("/home/") == 0 || path.find("/tmp/") == 0 ||
	    path.find("/var/") == 0 || path.find("/boot/") == 0)
		return false;
	return true;
}

// Generate error response with custom page if available, plain text otherwise
std::string Response::errorResponse(int statusCode, const std::string& message, const std::string& customErrorPage) {
	status = statusCode;
    body = message;
    
    // Use provided path OR auto-lookup from stored errorPages
    std::string errorPage = customErrorPage.empty() ? getErrorPagePath(statusCode) : customErrorPage;
    
    if (!errorPage.empty() && fileExists(errorPage)) {
        if (readFile(errorPage, body)) {
            headers["Content-Type"] = getContentType(errorPage);
            return buildHttpResponse();
        }
    }
    // Fallback to generic error page
	headers["Content-Type"] = "text/plain";
	return buildHttpResponse();
}
// Generate HTTP redirect response
std::string Response::redirectResponse(int code, const std::string& location) {
	status = code;
	body = "";
	headers["Location"] = location;
	headers["Content-Type"] = "text/plain";
	return buildHttpResponse();
}
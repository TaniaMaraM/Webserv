/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmaeda <kmaeda@student.42berlin.de>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 14:40:53 by kmaeda            #+#    #+#             */
/*   Updated: 2026/03/17 15:15:08 by kmaeda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/cgi/CGI.hpp"
#include "../../include/http/Request.hpp"
#include "../../include/http/Response.hpp"
#include "../../include/config/ServerConfig.hpp" 

CGI::CGI() {}

CGI::~CGI() {}

static volatile sig_atomic_t g_cgiTimedOut = 0;
static volatile sig_atomic_t g_cgiPid = -1;

static void cgiAlarmHandler(int) {
	g_cgiTimedOut = 1;
	if (g_cgiPid > 0)
		kill(g_cgiPid, SIGKILL);
}

std::string CGI::getQuery(const std::string& path) {
    size_t pos = path.find('?');
    return (pos != std::string::npos) ? path.substr(pos + 1) : "";
}

std::string CGI::getPathInfo(const std::string& requestPath, const std::string& scriptName) {
	size_t queryPos = requestPath.find('?');
	std::string pathOnly = (queryPos != std::string::npos) 
		? requestPath.substr(0, queryPos) 
		: requestPath;
	
	// Check if starts with scriptName
	if (pathOnly.find(scriptName) == 0) {
		return pathOnly.substr(scriptName.length());
	}
	
	return "";
}

char** CGI::createEnvArray() {
	char** envp = new char*[env.size() + 1];
	int i = 0;
	
	for (std::map<std::string, std::string>::const_iterator it = env.begin(); 
      it != env.end(); ++it) {
     std::string entry = it->first + "=" + it->second;
     envp[i] = new char[entry.length() + 1];
     std::strcpy(envp[i], entry.c_str());
     i++;
    }
	envp[i] = NULL;
	return envp;
}

void CGI::freeEnvArray(char** envp) {
    for (int i = 0; envp[i] != NULL; ++i) {
        delete[] envp[i];
    }
    delete[] envp;
}

static std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

static std::string toUpperUnderscore(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (c == '-') result += '_';
        else result += std::toupper(c);
    }
    return result;
}

void CGI::buildEnvironment(const Request& req, const ServerConfig& server, const std::string& scriptPath) {
    // Required CGI meta-variables
    env["REQUEST_METHOD"] = req.getMethod();
    env["QUERY_STRING"] = getQuery(req.getPath());
    env["SCRIPT_NAME"] = scriptPath;
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    
    // Server info
    env["SERVER_NAME"] = server.getServerName();
    env["SERVER_PORT"] = intToString(server.getPort());
    
    // Content headers (if present)
    std::string contentType = req.getHeader("Content-Type");
    if (!contentType.empty()) {
        env["CONTENT_TYPE"] = contentType;
    }
    
    if (req.getMethod() == "POST") {
        env["CONTENT_LENGTH"] = intToString(req.getBody().length());
    }
    
    // PATH_INFO
    env["PATH_INFO"] = getPathInfo(req.getPath(), scriptPath);
    
    // Convert all HTTP headers to HTTP_* variables
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string envName = "HTTP_" + toUpperUnderscore(it->first);
        env[envName] = it->second;
    }
}

std::string CGI::execute(const Request& req, const ServerConfig& server, const std::string& scriptPath, const std::string& interpreterPath) {
	int pipeIn[2], pipeOut[2];
	int status = 0;
	Response response;
	const unsigned int cgiTimeoutSec = 2;
	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
	 	return (response.errorResponse(500, "CGI: pipe() failed"));

	buildEnvironment(req, server, scriptPath);
	char** envp = createEnvArray();	
	
	char* args[3]; // Create args array for execve
	args[0] = const_cast<char*>(interpreterPath.c_str());
	args[1] = const_cast<char*>(scriptPath.c_str());
	args[2] = NULL;
	
	pid_t pid = fork();
	if (pid < 0) {
		freeEnvArray(envp);
		close(pipeIn[0]); close(pipeIn[1]);
		close(pipeOut[0]); close(pipeOut[1]);
		return (response.errorResponse(500, "CGI: fork() failed"));	
	}
	if (pid == 0) {
		if (dup2(pipeIn[0], STDIN_FILENO) == -1 || dup2(pipeOut[1], STDOUT_FILENO) == -1)
			exit(1);
		close(pipeIn[1]);
		close(pipeOut[0]);
		execve(interpreterPath.c_str(), args, envp);
		exit(1);
	}
	close(pipeIn[0]);
	close(pipeOut[1]);

	struct sigaction oldAct;
	struct sigaction act;
	std::memset(&act, 0, sizeof(act));
	act.sa_handler = cgiAlarmHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, &oldAct);
	g_cgiTimedOut = 0;
	g_cgiPid = pid;
	alarm(cgiTimeoutSec);
	// Write request body to CGI stdin with error checking
	ssize_t bodySize = static_cast<ssize_t>(req.getBody().length());
	if (bodySize > 0) {
		ssize_t written = write(pipeIn[1], req.getBody().c_str(), bodySize);
		if (written != bodySize) {
			close(pipeIn[1]);
			close(pipeOut[0]);
			waitpid(pid, &status, 0);
			freeEnvArray(envp);
			return (response.errorResponse(500, "CGI: failed to write request body"));
		}
	}
	close(pipeIn[1]);

	std::string output;
	char buffer[4096];
	ssize_t bytesRead;
	while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer))) > 0)
		output.append(buffer, bytesRead);
	bool readError = (bytesRead < 0);
	close(pipeOut[0]);
	alarm(0);
	sigaction(SIGALRM, &oldAct, NULL);
	g_cgiPid = -1;

	waitpid(pid, &status, 0);
	freeEnvArray(envp);
	if (g_cgiTimedOut)
		return (response.errorResponse(504, "CGI: timeout"));
	if (readError)
		return (response.errorResponse(500, "CGI: failed to read script output"));
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
		return (response.errorResponse(500, "CGI: script failed to execute properly"));
	if (WIFSIGNALED(status))
		return (response.errorResponse(504, "CGI: script killed (timeout or signal)"));
	return output;
}
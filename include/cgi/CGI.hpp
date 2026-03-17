/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmaeda <kmaeda@student.42berlin.de>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 14:40:38 by kmaeda            #+#    #+#             */
/*   Updated: 2026/03/17 15:06:30 by kmaeda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <cstring>
#include <cctype>
#include <iostream>
#include <sstream>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

class Request;
class ServerConfig;

class CGI {
	private:
		std::map<std::string, std::string> env;


		void buildEnvironment(const Request& req, const ServerConfig& server, const std::string& scriptPath);
		std::string getQuery(const std::string& path);
		std::string getPathInfo(const std::string& requestPath, const std::string& scriptName);
		char** createEnvArray();
		void freeEnvArray(char** envp); 
	public:
		CGI();
		~CGI();

		std::string execute(const Request& req, const ServerConfig& server, const std::string& scriptPath, const std::string& interpreterPath);
};

#endif

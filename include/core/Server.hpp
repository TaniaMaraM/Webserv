/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmaeda <kmaeda@student.42berlin.de>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 12:56:57 by kmaeda            #+#    #+#             */
/*   Updated: 2026/02/19 11:21:36 by kmaeda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <poll.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <map>
#include <set>
#include "Client.hpp"
#include "../http/Request.hpp"
#include "../http/Response.hpp"
#include "config/ServerConfig.hpp"

class Server {
	private:
		int listenFd;
		int port;
		std::map<int, Client> clients;
		ServerConfig* config;

		void closeClient(int cfd);

	public:
		Server(int port, ServerConfig* cfg);
		~Server();
		
		void setupListenSocket();
		void createPollFds(std::vector<struct pollfd>& pollFds, struct pollfd pfd);
		void acceptNewClients();
		void handleClientRead(int cfd, std::map<int, Client>::iterator it);
		int readClient(int cfd, std::map<int, Client>::iterator &it);

	void handleUnallowedMethod(Response &response, std::map<int, Client>::iterator &it);
		void handleMethod(Request &request, Response &response, const LocationConfig* matchedLocation, std::map<int, Client>::iterator &it);
		void handleClientWrite(int cfd, std::map<int, Client>::iterator it);
		void closeIfComplete(int cfd, std::map<int, Client>::iterator it);
		void checkTimeouts(time_t now, int timeoutSecs);

		std::map<int, Client>& getClients();
		void setListenFd(int newFd);
		int getListenFd() const;
		int getPort() const;
		ServerConfig* getConfig();
};

#endif

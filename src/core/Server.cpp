/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kmaeda <kmaeda@student.42berlin.de>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/02 12:57:49 by kmaeda            #+#    #+#             */
/*   Updated: 2026/02/19 11:19:45 by kmaeda           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/core/Server.hpp"

Server::Server(int port, ServerConfig* cfg) : listenFd(-1), port(port), config(cfg) {}

std::map<int, Client>& Server::getClients() {
	return clients;
}

void Server::setListenFd(int newFd) {
	listenFd = newFd;
}

int Server::getListenFd() const {
	return listenFd;
}

int Server::getPort() const {
	return port;
}

ServerConfig* Server::getConfig() {
	return config;
}

Server::~Server() {
	// Close listening socket if open
	if (listenFd != -1) {
		close(listenFd);
		listenFd = -1;
	}
	// Close all client sockets
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
		int cfd = it->first;
		close(cfd);
	}
	clients.clear();
}
/******************************************************************************/
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lusimon <lusimon@student.42heilbronn.de    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/14 16:05:22 by lusimon           #+#    #+#             */
/*   Updated: 2026/07/17 12:55:30 by lusimon          ###   ########.fr       */
/*                                                                            */
/******************************************************************************/

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <poll.h>

class Socket
{
	private:
	    pollfd poll;
	    Client* client;

	public:
	    bool isClient() const;
};

// Listening socket:

// Socket
//  |
//  |-- poll.fd = 3
//  |
//  |-- client = NULL

// Socket listen_socket;

// listen_socket.poll.fd = listen_fd;
// listen_socket.client = NULL;

// sockets.push_back(listen_socket);


// Client socket:

// Socket
//  |
//  |-- poll.fd = 5
//  |
//  |-- client ---> Client object

// Socket new_socket;

// new_socket.poll.fd = client_fd;
// new_socket.client = new Client(client_fd);

// sockets.push_back(new_socket);



struct Client
{
	int fd;
	std::string buffer;
	bool request_complete;
	bool keep_alive;
	
	Client(int socket_fd)
	{
		fd = socket_fd;
		buffer = "";
		request_complete = false;
		keep_alive = true;
	}
};

#endif


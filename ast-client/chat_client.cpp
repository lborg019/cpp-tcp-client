//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctype.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

using boost::asio::ip::tcp;

enum { buffer_size = 8 };

std::string check_file(std::string message, size_t request_length, std::shared_ptr<boost::asio::ip::tcp::socket> s)
{
	boost::asio::streambuf receive_buffer;
	boost::system::error_code error;
	char rcv_buf[buffer_size];	// 8 byte buffer for message size
	std::string response;		// store server response here

	//send message size as an 8 byte message:
	boost::asio::write(*s, boost::asio::buffer(std::to_string(request_length), buffer_size));
	std::cout << "waiting to receive ack...\n";

	// receive ack
	size_t len = s->read_some(boost::asio::buffer(rcv_buf), error);
	std::string received_ack(rcv_buf[0], sizeof(rcv_buf));
	received_ack.resize(len);
	if (error && error != boost::asio::error::eof) {
		std::cout << "error status: " << error.message() << std::endl;
	}
	else
	{
		std::string reply(rcv_buf);
		reply = reply.substr(0, 3);
		std::cout << reply << ": server ackd\n";
	}

	// send message content
	boost::algorithm::trim_left(message);
	boost::asio::write(*s, boost::asio::buffer(message, message.length()));

	// receive message size:
	int remaining_data = 0;
	rcv_buf[0] = '\0';
	len = s->read_some(boost::asio::buffer(rcv_buf), error);
	std::string received_size(rcv_buf[0], sizeof(rcv_buf));
	received_size.resize(len);
	if (error && error != boost::asio::error::eof) {
		std::cout << "error status: " << error.message() << std::endl;
	}
	else {
		std::string reply(rcv_buf);
		std::string s = boost::regex_replace(
			reply,
			boost::regex("[^0-9]*([0-9]+).*"),
			std::string("\\1")
		);
		std::cout << "receiving: " << s << " bytes\n";

		std::string::size_type sz;
		remaining_data = std::stoi(s, &sz); // save it as integer
	}

	// receive message content:
	rcv_buf[0] = '\0'; // clean buffer
	if (remaining_data >= 260) // big message, write to file:
	{
		std::cout << "REMAINING_DATA: " << remaining_data << "\n";
		std::cout << "receiving .json file\n";

		// format file name
		// ../files/file1.py
		std::string delim_1 = "/";
		std::string delim_2 = ".";

		std::string file_name = message.substr(message.rfind(delim_1) + 1);
		file_name = file_name.substr(0, file_name.rfind(delim_2));
		file_name = file_name.append(".json");
		std::cout << "writing to file_name: " << file_name << "\n";

		//delete if exists so we do not append:
		std::remove(&file_name[0]);

		if (std::remove(&file_name[0]) != 0)
			perror("Error deleting existing file. Information will append\n");
		else
			puts("Overwriting existing file\n");

		//open binary file
		//										    output          append            binary
		std::ofstream outputfile(file_name, std::ios::out | std::ios::app | std::ios::binary);
		while (remaining_data != 0)
		{
			//receive slab 8 byte or bigger:
			if (remaining_data >= 8) {
				len = s->read_some(boost::asio::buffer(rcv_buf, 8), error);
				if (error && error != boost::asio::error::eof) {
					std::cout << "error status: " << error.message() << std::endl;
				}
				else
				{
					//traverse the buffer, catch each byte and write to binary file:
					for (size_t i = 0; i < 8; i++)
					{
						outputfile.write(&rcv_buf[i], sizeof(char));
					}
					remaining_data = remaining_data - len;
					std::cout << "wrote (big): " << len << " bytes. remaining_data: " << remaining_data << "\n";
				}
			}
			else //receive slab smaller than 8 bytes
			{
				len = s->read_some(boost::asio::buffer(rcv_buf, remaining_data), error);
				if (error && error != boost::asio::error::eof) {
					std::cout << "error status: " << error.message() << std::endl;
				}
				else
				{
					for (size_t i = 0; i < (size_t)remaining_data; i++)
					{
						outputfile.write(&rcv_buf[i], sizeof(char));
					}
					remaining_data = remaining_data - len;
					std::cout << "wrote (small): " << len << " bytes. remaining_data: " << remaining_data << "\n";
				}
			}
		}
		outputfile.close();
	}
	else // receive regular echo message:
	{
		std::cout << "receiving regular message\n";
		while (remaining_data != 0)
		{
			if (remaining_data >= buffer_size)
			{
				// receive slab 8 byte or bigger
				len = s->read_some(boost::asio::buffer(rcv_buf, buffer_size), error);
				if (error && error != boost::asio::error::eof) {
					std::cout << "error status: " << error.message() << std::endl;
				}
				else {
					std::string reply(rcv_buf);
					reply = reply.substr(0, len);
					response.append(reply);
					std::cout << "wrote: " << len << " bytes\n";
					remaining_data = remaining_data - len;
				}
			}
			else // receive slab smaller than 8 bytes
			{
				// read entire buffer, shrink correct size afterwards
				len = s->read_some(boost::asio::buffer(rcv_buf, buffer_size), error);
				if (error && error != boost::asio::error::eof) {
					std::cout << "error status: " << error.message() << std::endl;
				}
				else
				{
					std::string reply(rcv_buf);
					reply = reply.substr(0, remaining_data - 1);
					std::cout << "wrote: " << len << " bytes\n";
					response.append(reply);
					remaining_data = remaining_data - len;
					//clean buffer maybe?
				}
			}
		}
	}
	std::cout << "response: " << response << "\n";
	return response;
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 3)
		{
			std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
			return 1;
		}

		boost::asio::io_service io_service;

		// create a shared ptr so we can pass the socket to a function:
		std::shared_ptr<boost::asio::ip::tcp::socket> current_socket;
		current_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service);

		// pass command line arguments instead of hard-coding:
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(argv[1]),
												boost::lexical_cast<unsigned>(argv[2]));
		boost::system::error_code ec;
		current_socket->connect(endpoint, ec);

		std::string message;		// store client input here
		std::string response;		// store server response here

		for (;;)
		{
			std::cout << "Enter message: ";
			getline(std::cin, message);
			message.append("\r");		// append message end to the message
			size_t request_length = message.length(); // calculate the size of the message
			response = check_file(message, request_length, current_socket); // empty string if something goes wrong
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	//system("PAUSE");
	return 0;
}
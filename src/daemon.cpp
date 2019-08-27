#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <vector>

// fork
#include <sys/types.h>
#include <unistd.h>

#include "Logger.hpp"
#include "Socket.hpp"
#include "CommandLine.hpp"

#include <cstring>

using est::CommandLine;
using est::socket::FileDescriptor;
using est::socket::UnixDomainListenSocket;

void daemonize() {
	auto pid = fork();
	switch (pid) {
		case 0:
			exit(0);
			break;
		case -1:
			LOG_ERROR("Unable to fork");
			exit(1);
			break;
		default:
			LOG_INFO("Process forked");
			break;
	}
}

class SafeServiceList {
       public:
	using UniqueId = std::pair<int, int>;

	void Insert(const UniqueId& key, int fd) {
		std::lock_guard<std::mutex> lock(write_mutex_);
		services_.insert(std::make_pair(key, fd));
	}

	int Get(const UniqueId& key) {
		std::lock_guard<std::mutex> lock(write_mutex_);
		// std::lock_guard<std::mutex> lock(read_mutex_);

		auto it = services_.find(key);
		if (it != services_.end())
			return it->second;
		else
			return -1;
	}

       private:
	std::map<UniqueId, FileDescriptor> services_;

	std::mutex read_mutex_;
	std::mutex write_mutex_;
};

class SafeCallList {};

class Packet {
       public:
	Packet() = default;
	Packet(const char* buf) { FromBuffer(buf); }

	enum class Type { REGISTER = 'a', UNREGISTER = 'b', CALL = 'c'} type;
	int service;
	int method;

	void FromBuffer(const char* buf) {
		type = static_cast<Type>(buf[0]);
		service = static_cast<int>(buf[1]);
		method = static_cast<int>(buf[2]);
	}

	auto GetUuid() {
		return std::make_pair(service, method);
	}

	auto ToBuffer() {
	return std::array<char, 3>{static_cast<char>(type), static_cast<char>(service), static_cast<char>(method)};
	}

	friend std::ostream& operator<<(std::ostream& stream, const Packet& p);
};

std::ostream& operator<<(std::ostream& stream, const Packet& p) {
	char buf[60];
	std::string type;
	switch (p.type) {
		case Packet::Type::REGISTER:
			type = "REGISTER";
			break;
		case Packet::Type::UNREGISTER:
			type = "UNREGISTER";
			break;
		case Packet::Type::CALL:
			type = "CALL";
			break;
	}

	sprintf(buf, "Packet (type = %s, service = %d, method = %d)",
		type.c_str(), p.service, p.method);

	return stream << buf;
}

int main(int argc, char** argv) {
	// CommandLine cl(argc, argv);
	CommandLine cl;
	cl.AddArgument("--buffer-size", CommandLine::ArgumentType::INT);
	cl.Parse(argc, argv);

	bool verbose = false;
	if (cl.IsSet("-v") || cl.IsSet("--verbose")) verbose = true;

	if (cl.IsSet("-d") || cl.IsSet("--daemonize")) daemonize();

	SafeServiceList ssl;
	auto accept_handler =
	    std::make_shared<UnixDomainListenSocket::AcceptCallback>(
		[&ssl](int _fd) {
			LOG_DEBUG("Inside Accept callback");
			// FileDescriptor fd(_fd);

			char buf[1024];
			int total_bytes_read = 0;
			int last_read = 0;
			last_read = read(_fd, buf, sizeof(buf));
				total_bytes_read += last_read;

			if (last_read == -1) {
				return;
			}
        LOG_INFO("Read inside callback"); printf("bytes = %d\n", total_bytes_read);

			write(_fd, static_cast<const void*>(&"Packet received"), 15);
			Packet p(buf); memset(buf, 0, sizeof(buf));
			switch (p.type) {
				case Packet::Type::REGISTER:
				ssl.Insert(p.GetUuid(), _fd);
				break;
				case Packet::Type::UNREGISTER:
				break;
				case Packet::Type::CALL: {
				int fd = ssl.Get(p.GetUuid());
				auto buf2 = p.ToBuffer();
				write(fd, reinterpret_cast<const void*>(&buf2[0]), buf2.size());
				total_bytes_read = 0;
				last_read = 0;
				last_read = read(fd, buf, sizeof(buf));
					total_bytes_read += last_read;

				if (last_read == -1) {
					write(_fd, static_cast<const void*>(&"fail"), 4);
				} else {
					write(_fd, static_cast<const void*>(&"success"), 7);
				}
				break;
				}
				default:
				break;
			}

			std::cout << p << '\n';
		});

	const std::string listen_file("/tmp/socket-foo");
	const int max_waiting_jobs = 10;
	UnixDomainListenSocket sock(listen_file, max_waiting_jobs);

	// TODO fix log and use it
	std::cout << "Starting up ipc-thingy-d....." << std::endl;
	std::cout << "Resolving calls on " << listen_file << std::endl;

	sock.OnAccept(std::move(accept_handler));

	while (true) {
		sock.BlockingListen();
	}

	return 0;
}


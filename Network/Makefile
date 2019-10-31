all:tcp_cli tcp_srv udp_srv udp_cli
udp_cli:udp_cli.cpp
		g++ $^ -o $@
tcp_cli:tcp_cli.cpp
		g++ $^ -o $@
tcp_srv:tcp_srv.cpp
		g++ $^ -o $@
udp_srv:udp_srv.c
		gcc $^ -o $@


.PHONY:ALL
ALL: mysql_cgi test_cgi

mysql_cgi:mysql_cgi.cc
	g++ -o $@ $^ -std=c++11 -I include -L lib -lmysqlclient -lpthread -ldl
test_cgi:test_cgi.cc
	g++ -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f mysql_cgi test_cgi

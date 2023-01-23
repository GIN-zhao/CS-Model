SRC=main.cpp
TARGET=http_server
CC=g++
FLAGS=-std=c++11 -lpthread #-g


$(TARGET):$(SRC)
	$(CC) -o $@ $^ $(FLAGS)

.PHONY:clean
clean:
	rm -rf output
	rm -f $(TARGET)

.PHONY:output
output:
	mkdir output
	cp $(TARGET) output
	mv ./cgi/test_cgi  wwwroot
	mv ./cgi/mysql_cgi  wwwroot
	cp -rf wwwroot output

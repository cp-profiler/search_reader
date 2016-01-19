OBJS = search_reader.o message.pb.o connector.o
CC = g++
DEBUG = -g
CFLAGS = -std=c++0x -O2 -W -Wall -c $(DEBUG)
GCC_FLAGS = -O2 -W -Wall -c $(DEBUG)
LFLAGS = -W -Wall $(DEBUG)

search_reader: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -lpthread -ldl -L/usr/local/lib -lprotobuf -o search_reader
search_reader.o: search_reader.cpp
	$(CC) $(CFLAGS) search_reader.cpp
connector.o : cpp-integration/connector.cpp
	$(CC) $(CFLAGS) cpp-integration/connector.cpp
message.pb.o: message.pb.cpp
	$(CC) -O2 -c message.pb.cpp -o message.pb.o

clean:
	rm *.o search_reader

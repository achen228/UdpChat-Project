CFLAGS=-std=c++11 -g
LDFLAGS=-lpthread -ljsoncpp

SVRBIN=../bin/svr
CLIBIN=../bin/cli

.PHONY:all
all:$(SVRBIN) $(CLIBIN)
$(SVRBIN):chatsvr.cpp 
	g++ $^ -o $@ $(CFLAGS) $(LDFLAGS)
$(CLIBIN):chatcli.cpp 
	g++ $^ -o $@ $(CFLAGS) $(LDFLAGS) -lncurses

.PHONY:clean
clean:
	rm -f $(SVRBIN) $(CLIBIN)

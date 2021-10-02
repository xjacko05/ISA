SOURCE = mytftpclient
CC = g++
all: $(SOURCE).cpp
	@$(CC) -Wall -Wextra -pedantic -o $(SOURCE) $(SOURCE).cpp
clean: $(SOURCE)
	@rm $(SOURCE)

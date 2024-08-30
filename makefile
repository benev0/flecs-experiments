default:
	make server
	make client

server:
	(cd src && make server)
	(cd bin && ./server)

client:
	(cd src && make client)
	(cd bin && ./client)

clean:
	(cd bin && rm -f server)
	(cd bin && rm -f client)
	(cd obj && rm -f **/**/*.o **/*.o *.o)

# fixme later
# grind: app
# 	valgrind --leak-check=summary --show-reachable=no --show-leak-kinds=all ./app

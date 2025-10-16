client.o: src/client.c src/client.h src/main.h src/file_p.h src/session.h
file_p.o: src/file_p.c src/file_p.h src/main.h src/session.h
main.o: src/main.c src/main.h src/file_p.h src/server.h
server.o: src/server.c src/main.h src/session.h src/user.h
session.o: src/session.c src/session.h src/main.h src/client.h \
 src/file_p.h src/user.h
user.o: src/user.c src/main.h src/session.h

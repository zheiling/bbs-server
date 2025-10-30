client.o: src/client.c src/client.h src/main.h src/file_p.h src/session.h
db.o: src/db.c src/db.h src/main.h
file_p.o: src/file_p.c src/file_p.h src/main.h src/libs/murmur3/murmur3.h
main.o: src/main.c src/main.h src/file_p.h src/server.h src/db.h
server.o: src/server.c src/main.h src/session.h src/user.h
session.o: src/session.c src/session.h src/main.h src/client.h \
 src/file_p.h src/user.h src/db.h
user.o: src/user.c src/main.h src/session.h src/db.h

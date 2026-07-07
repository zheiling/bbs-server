#include <arpa/inet.h>
#include <cmocka.h>
#include <main.h>
#include <netinet/in.h>
#include <server.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define CLIENT_PORT PORT + 1

int sd = -1;
int lpid = -1;
int _addr;
volatile sig_atomic_t child_ready = 0;
#define WELCOME_MES "Hello everyworld!\n"

int init_client(void);
bool connect_to_server(int sd, int addr, short int port);
void _server_loop();
void usr1hdl(int n) { child_ready = true; }
int setup(void **state);
int tear_down(void **state);

/* === TESTS === */

/* TODO: better solution than sleep() */
void test__welcome(void **state) {
  char str[] = WELCOME_MES "login> ";
  char buf[256];
  int rlen = read(sd, buf, 128);
  buf[rlen] = 0;
  assert_string_equal(str, buf);
}

/* === MAIN === */
int main(int argc, char **argv) {
  const struct CMUnitTest tests[] = {cmocka_unit_test(test__welcome)};
  return cmocka_run_group_tests(tests, setup, tear_down);
}

/* === ADDITIONAL === */
bool connect_to_server(int sd, int addr, short int port) {
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = addr;
  server.sin_port = port;

  if (-1 == connect(sd, (struct sockaddr *)&server, sizeof(server))) {
    return false;
  }

  return true;
}

int init_client(void) {
  int res;
  int cs = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in client;

  client.sin_family = AF_INET;
  client.sin_port = htons(CLIENT_PORT);
  client.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt = 1;
  setsockopt(cs, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  res = bind(cs, (struct sockaddr *)&client, sizeof(client));
  if (res == -1) {
    exit(1);
  }
  return cs;
}

void _server_loop() {
  server_data_t s_d = {.ls = start_server(),
                       .welcome_message = WELCOME_MES};
  if (s_d.ls == -1) {
    kill(getppid(), SIGTERM);
    exit(1);
  }
  kill(getppid(), SIGUSR1);
  server_main_loop(&s_d);
  exit(0);
}

int setup(void **state) {
  sigset_t mask_usr1, mask_empty;
  sigemptyset(&mask_usr1);
  sigaddset(&mask_usr1, SIGUSR1);
  sigemptyset(&mask_empty);
  sigprocmask(SIG_SETMASK, &mask_usr1, NULL);
  signal(SIGUSR1, usr1hdl);

  lpid = fork();

  if (lpid == 0) {
    _server_loop();
  }

  while (!child_ready)
    sigsuspend(&mask_empty);
  struct sockaddr_in addr;
  inet_aton("127.0.0.1", &(addr.sin_addr));
  _addr = addr.sin_addr.s_addr;

  sd = init_client();
  int res = connect_to_server(sd, _addr, htons(PORT));
  if (!res) {
    perror("connect failed");
    kill(lpid, SIGTERM);
    exit(1);
  }
  return 0;
}

int tear_down(void **state) {
  kill(lpid, SIGTERM);
  return 0;
}

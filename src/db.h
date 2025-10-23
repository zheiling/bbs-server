typedef struct {
    char *name;
    char *pass;
} i_auth_t;

typedef struct {
    char privileges;
    int is_logged;
} o_auth_t;

int init_connection();
int db_user_auth(i_auth_t *credentials, o_auth_t *response);

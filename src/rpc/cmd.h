#ifndef cmd_h_INCLUDED
#define cmd_h_INCLUDED

enum {
    CMD_SUCCESS
    CMD_EMSGLEN,
};

int cmd_perform(const char *cmd, size_t len);

#endif // cmd_h_INCLUDED

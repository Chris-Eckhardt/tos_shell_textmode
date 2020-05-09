/********************************************
 * 
 * TOS Shell
 * 
 * Name: Chris Eckhardt
 * 
 * ID: 915736372
 * 
 * *****************************************/


#include <kernel.h>


#define SHELL_CMD_INVALID  -1
#define SHELL_CMD_HELP      0
#define SHELL_CMD_CLEAR     1
#define SHELL_CMD_SHELL     2
#define SHELL_CMD_PONG      3
#define SHELL_CMD_ECHO      4
#define SHELL_CMD_PS        5
#define SHELL_CMD_ABOUT     6

#define SHELL_ENTER         0x0D
#define SHELL_BACKSPACE     0x08
#define SHELL_EOF           0x10
#define SHELL_BUFF_SIZE     256

typedef struct _CMD {
    int    window_id;
    int    command_id;
    char * buffer;
    char * command;
    char * param;
} CMD;

/******************************************
 *           Function Prototypes          *
 * ****************************************/
// shell related functions will have the prefix 'sh_[descriptive function name]'

void shell_process( PROCESS self, PARAM param );

void start_shell ();

CMD * sh_init_cmd ( int width, int height);

void sh_delete_cmd( CMD * cmd);

void sh_get_line ( CMD * cmd );

void sh_parse ( CMD * cmd );

void sh_execute ( CMD * cmd );

void sh_clear_cmd_buffers ( CMD * cmd );

int sh_command_comp (char * token);

int sh_strcmp (char * string1, char * string2);

void sh_tokenize ( CMD * cmd );

void sh_help_print  ( int window_id );

void sh_print_processes (int window_id);

void sh_print_processes_detailed (int window_id);

void sh_print_processes_detailed_helper(int window_id, PROCESS p);


/******************************************
 *               start shell              *
 * ****************************************/
// All start_shell() does is call create process

void start_shell ()
{
    create_process(shell_process, 5, 0, "shell_process");
}

/******************************************
 *             Shell Process              *
 * ****************************************/
// This is the main loop of the shell. 
// 1. It clears the command buffers.
// 2. Waits for the user to type a command. 
// 3. Parses, tokenizes, and validates the input.
// 4. then it executes the command (or prints error statement)

void shell_process (PROCESS self, PARAM param)
{
    CMD * cmd = sh_init_cmd(50, 17);
    while (1) {
        sh_clear_cmd_buffers(cmd);
        wm_print(cmd->window_id, "> ");
        sh_get_line(cmd);
        sh_parse(cmd);
        sh_execute(cmd);
    }
    sh_delete_cmd(cmd);
    become_zombie();
}

/******************************************
 *    initialize & free command struct    *
 * ****************************************/
// These functions just make the code in shell_process
// much cleaner and shorter. 

CMD * sh_init_cmd (int width, int height)
{
    CMD * cmd = (CMD *) malloc(sizeof(CMD));
    cmd->command_id = SHELL_CMD_INVALID;
    cmd->buffer = (char *) malloc(SHELL_BUFF_SIZE);
    cmd->command = (char *) malloc(SHELL_BUFF_SIZE);
    cmd->param = (char *) malloc(SHELL_BUFF_SIZE);
    cmd->window_id = wm_create(10, 3, width, height);
    return cmd;
}

void sh_delete_cmd ( CMD * cmd )
{
    free(cmd->buffer);
    free(cmd->command);
    free(cmd->param);
    cmd->buffer = NULL;
    cmd->command = NULL;
    cmd->param = NULL;
    free(cmd);
    cmd = NULL;
}

/******************************************
 *               sh_get_line              *
 * ****************************************/
// get keystrokes from keyboard, emplace chars in command buffer 
// until the return key is pressed or char limit is reached.

void sh_get_line ( CMD * cmd )
{
    char ch;
    int i = 0;
    while ( (ch = keyb_get_keystroke(cmd->window_id, TRUE)) != SHELL_ENTER) {
        if (ch == SHELL_BACKSPACE) {
            if (i > 0){
                cmd->buffer[--i] = 0;
                wm_print(cmd->window_id, "%c", ch);
            }
        } else {
            cmd->buffer[i++] = ch;
            wm_print(cmd->window_id, "%c", ch);
        }
        if (i > SHELL_BUFF_SIZE-1)
            break;
    }
    cmd->buffer[i++] = '\0';
    wm_print(cmd->window_id, "\n");
}

/******************************************
 *               sh_parse                 *
 * ****************************************/
// this function calls tokenize, and then validates the tokens
// in cmd->command and cmd->param (cmd->param hols any arguments passed
// with the command)

void sh_parse (CMD * cmd)
{
    sh_tokenize(cmd);
    cmd->command_id = sh_command_comp(cmd->command);
    if (cmd->command_id != SHELL_CMD_ECHO && cmd->command_id != SHELL_CMD_PS) {
        if (k_strlen(cmd->param) != 0)
            cmd->command_id = SHELL_CMD_INVALID;
        
    } else {
        if (cmd->command_id == SHELL_CMD_PS)
            if (k_strlen(cmd->param) != 0) 
                if (sh_strcmp(cmd->param, "-d") != 0) 
                    cmd->command_id = SHELL_CMD_INVALID;
    }
}

/******************************************
 *               sh_execute               *
 * ****************************************/
// this does as its name suggests and executes the command.

void sh_execute ( CMD * cmd )
{
    switch (cmd->command_id) {

        case SHELL_CMD_INVALID:
            wm_print(cmd->window_id, "invalid command: %s\n", cmd->command);
            wm_print(cmd->window_id, "Type 'help' for available commands\n");
            break;
        case SHELL_CMD_HELP:
            sh_help_print(cmd->window_id);
            break;
        case SHELL_CMD_CLEAR:
            wm_clear(cmd->window_id);
            break;
        case SHELL_CMD_SHELL:
            start_shell();
            break;
        case SHELL_CMD_PONG:
            start_pong();
            break;
        case SHELL_CMD_ECHO:
            wm_print(cmd->window_id, "%s\n", cmd->param);
            break;
        case SHELL_CMD_PS:
            if (k_strlen(cmd->param) == 0) {
                sh_print_processes(cmd->window_id);
            } else {
                sh_print_processes_detailed(cmd->window_id);
            }
            break;
        case SHELL_CMD_ABOUT:
            wm_print(cmd->window_id, "Chris Eckhardt really, really likes programming.\n");
            break;
    }
}

/******************************************
 *         sh_clear_cmd_buffers           *
 * ****************************************/
// this is called at the beginning of every iteration 
// inside the while loop of shell_process. all it does
// is fills the command buffers with zeros

void sh_clear_cmd_buffers(CMD * cmd) 
{
    cmd->command_id = SHELL_CMD_INVALID;
    for (int i = 0; i < SHELL_BUFF_SIZE-1; i++)
        cmd->buffer[i] = 0;
    for (int i = 0; i < SHELL_BUFF_SIZE-1; i++)
        cmd->command[i] = 0;
    for (int i = 0; i < SHELL_BUFF_SIZE-1; i++)
        cmd->param[i] = 0;
}

/******************************************
 *               sh_strcmp                *
 * ****************************************/
// This is not my code. I dont know if this is the 
// standard C implementation of strcmp but it is a 
// commonly used one, and I use it all the time.

int sh_strcmp(char * string1, char * string2 )
{
    for (int i = 0; ; i++)
    {
        if (string1[i] != string2[i])
        {
            return string1[i] < string2[i] ? -1 : 1;
        }

        if (string1[i] == '\0' || string1[i] == 0)
        {
            return 0;
        }
    }
}

/******************************************
 *           sh_command_comp              *
 * ****************************************/
// This is not my code. I dont know if this is the 
// standard C implementation of strcmp but it is a 
// commonly used one, and I use it all the time.

int sh_command_comp(char * token)
{
    char * possible_commands[7] = { "help", "clear", "shell", "pong", "echo", "ps", "about" };

    for (int i = 0; i < 7; i++)
    {
        if (sh_strcmp(token, possible_commands[i]) == 0) {
            return i;
        }
    }
    return SHELL_CMD_INVALID;
}

/******************************************
 *               sh_tokenize                *
 * ****************************************/
// This function goes through each character in the cmd->buffer (which holds the raw user input)
// and tokenizes the string, the first token (the command) goes into the cmd->command buffer
// the second token (the parameter, if there is one) goes into cmd->param
// note: tokens are delimited by spaces

void sh_tokenize ( CMD * cmd )
{
    int flag1 = FALSE;
    int flag2 = FALSE;
    int flag3 = FALSE;
    int flag4 = FALSE;
    int b_index = 0;
    int c_index = 0;
    int p_index = 0;
    while (cmd->buffer[b_index] != '\0') {
        if (cmd->buffer[b_index] != ' ' && !flag1) {
            flag1 = TRUE;
        }
        if (cmd->buffer[b_index] == ' ' && flag1 && !flag2) {
            flag2 = TRUE;
        }
        if (cmd->buffer[b_index] != ' ' && flag1 && flag2 && !flag3) {
            flag3 = TRUE;
        }
        if (cmd->buffer[b_index] != ' ' && flag1 && !flag2) {
            cmd->command[c_index] = cmd->buffer[b_index];
            c_index++;
        }
        if (flag3) {
            cmd->param[p_index] = cmd->buffer[b_index];
            p_index++;
        }
        b_index++;
    }
    cmd->command[c_index] = '\0';
    cmd->param[p_index] = '\0';
}

/******************************************
 *          ps helper functions           *
 * ****************************************/
// These functions are basically your print_all_processes function 
// but with minor variations to fit their intended purposes.

void sh_help_print (int window_id)
{
    wm_print(window_id, "TOS shell commands:\n");
    wm_print(window_id, "  help : lists available commands\n");
    wm_print(window_id, "  clear : clear the shell\n");
    wm_print(window_id, "  shell : create a new shell\n");
    wm_print(window_id, "  pong : play a game of pong\n");
    wm_print(window_id, "  echo <text> : print text to the shell\n");
    wm_print(window_id, "  ps [-d] : list running processes\n");
    wm_print(window_id, "  about : a message the from author\n");
}

void sh_print_processes (int window_id) 
{
    int i;
    PCB * p = pcb;

    wm_print(window_id, "    Name\n");
    wm_print(window_id, "------------------------------------------------\n");
    for (i = 0; i < MAX_PROCS; i++, p++) {
        if (!p->used)
            continue;
        wm_print(window_id, "    %s\n", p->name);
    }
}

void sh_print_processes_detailed (int window_id) 
{
    int i;
    PCB * p = pcb;

    wm_print(window_id, "State           Active Prio Name\n");
    wm_print(window_id, "------------------------------------------------\n");
    for (i = 0; i < MAX_PROCS; i++, p++) {
        if (!p->used)
            continue;
        sh_print_processes_detailed_helper(window_id, p);
    }
}

void sh_print_processes_detailed_helper (int window_id, PROCESS p)
{
    static const char *state[] = { "READY          ",
        "ZOMBIE         ",
        "SEND_BLOCKED   ",
        "REPLY_BLOCKED  ",
        "RECEIVE_BLOCKED",
        "MESSAGE_BLOCKED",
        "INTR_BLOCKED   "
    };
    if (!p->used) {
        wm_print(window_id, "PCB slot unused!\n");
        return;
    }
    /* State */
    wm_print(window_id, state[p->state]);
    /* Check for active_proc */
    if (p == active_proc)
        wm_print(window_id, " *      ");
    else
        wm_print(window_id, "        ");
    /* Priority */
    wm_print(window_id, "  %2d", p->priority);
    /* Name */
    wm_print(window_id, " %s\n", p->name);
}
 


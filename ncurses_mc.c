#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>
#include <curses.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <dirent.h>

#define _KEY_EXIT	'q'		
#define _KEY_ENTER	0xa
#define _KEY_TAB	0x9	

#define OPEN_HELP_TXT          "Press ENTER key to open dir"
#define CHANGE_HELP_TXT        "Press TAB key to change window"
#define NAVIGATE_HELP_TXT      "Press up/down arrow key to navigate"
#define OPEN_ANOTHER_HELP_TXT  "Press left/right arrow key to open dir in another window"
#define EXIT_HELP_TXT          "Press q key to exit"
#define HELP_TXT_NUMBER        5

#define WINDOW_STRINGS 30
#define WINDOW_COLUMNS 40

#define S_IFIFO_TXT     "is a named pipe (FIFO)"
#define S_IFCHR_TXT     "is a character device"
#define S_IFDIR_TXT     "is a directory"
#define S_IFBLK_TXT     "is a block device"
#define S_IFREG_TXT     "is a regular file"
#define S_IFLNK_TXT     "is a symbolic link"
#define S_IFSOCK_TXT    "is a UNIX domain socket"
#define S_UNKNOWN_TXT   "file type is unknown"
#define OPEN_ERR_TXT    "Permission denied"

#define CURRENT_DIR     "."
#define PREVIOUS_DIR    ".."

#define RETURN_OK  (0)
#define RETURN_ERR (1)

enum windows {
    first_wnd = 0,
    second_wnd,
    all_wnd
};


int check_file_type(char *dir, char *file);
void update_current_dir(struct dirent ** namelist, int file, char *dir);
void update_window(WINDOW * wnd, struct dirent ** namelist, int files);
void print_error_msg(char *error_msg);

struct winsize size;
int window_strings, window_columns;

void sig_winch(int signo)
{
    struct winsize size;
    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
    resizeterm(size.ws_row, size.ws_col);
}

int no_rule(const struct dirent * d)
{
    return 1;
}

int main()
{  
    WINDOW * wnd[all_wnd];
    WINDOW * subwnd[all_wnd];
    char *error_msg;
    int i = 0, j = 0, files_number[all_wnd], ret;
    int cursore_string, cursore_column;    
    int input_simbol, current_file = 0, current_wnd = 0;
    char current_dir[all_wnd][1000];    
    struct dirent ** namelist[all_wnd];
    const char *help_txt[HELP_TXT_NUMBER] = {OPEN_HELP_TXT, CHANGE_HELP_TXT, NAVIGATE_HELP_TXT, OPEN_ANOTHER_HELP_TXT, EXIT_HELP_TXT};  

    ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
    window_columns = size.ws_col/2 - 4;
    window_strings = size.ws_row - HELP_TXT_NUMBER - 5;

    initscr();  
    signal(SIGWINCH, sig_winch);                 
    curs_set(TRUE);
    start_color();
    refresh();

    j = window_strings + 5;
    for(i = 0; i < HELP_TXT_NUMBER; i++)
    {
        move(j++, window_columns+2 - strlen(help_txt[i])/2);
        printw("%s", help_txt[i]);
    }
    refresh();

    wnd[first_wnd] = newwin(window_strings+2, window_columns+2, 1, 1);
    wnd[second_wnd] = newwin(window_strings+2, window_columns+2, 1, window_columns+2);               
    keypad(stdscr, TRUE);

    for(i = 0; i < all_wnd; i++)
    {
        box(wnd[i], '|', '-');
        subwnd[i] = derwin(wnd[i], window_strings, window_columns, 1, 1);
        keypad(wnd[i], TRUE);
        getcwd(current_dir[i], sizeof(current_dir[i]));
        files_number[i] = scandir(current_dir[i], &namelist[i], no_rule, alphasort);
        if (files_number[i] < 0)
        {
            perror("scandir() ");
            return 1;
        } 
        for (j = 0; j < files_number[i]; j++) 
        {
            wprintw(subwnd[i], "%s\n", namelist[i][j]->d_name);
        }
        wrefresh(wnd[i]);
        wmove(subwnd[i], 0, 0); 
        wrefresh(subwnd[i]);    
    }
   
    while(1)
    {
        input_simbol = getch();      
        if(input_simbol == _KEY_EXIT)
            break;

        switch (input_simbol)
        {
            case _KEY_TAB:
                current_wnd = current_wnd ? first_wnd : second_wnd; 
                wmove(subwnd[current_wnd], 0, 0);
                wrefresh(subwnd[current_wnd]);
                break;
            case KEY_DOWN:
                getyx(subwnd[current_wnd], cursore_string, cursore_column);
                if(cursore_string+1 < files_number[current_wnd]) 
                {
                    wmove(subwnd[current_wnd], ++cursore_string, cursore_column);
                    wrefresh(subwnd[current_wnd]);
                }
                break; 
            case KEY_UP:
                getyx(subwnd[current_wnd], cursore_string, cursore_column);
                wmove(subwnd[current_wnd], --cursore_string, cursore_column);
                wrefresh(subwnd[current_wnd]);
                break;  
            case KEY_RIGHT:
                if(current_wnd == second_wnd)
                    break;   

                if(cursore_string > files_number[current_wnd])
                    break;   
                current_file = cursore_string;

                if(check_file_type(current_dir[current_wnd], namelist[current_wnd][current_file]->d_name))
                    break;               
                strcpy(current_dir[second_wnd], current_dir[first_wnd]);       
                update_current_dir(namelist[current_wnd], current_file, current_dir[second_wnd]);
                current_wnd = second_wnd; 

                while (files_number[current_wnd]--) 
                {
                    free(namelist[current_wnd][files_number[current_wnd]]);
                }
                free(namelist[current_wnd]);

                files_number[current_wnd] = scandir(current_dir[current_wnd], &namelist[current_wnd], no_rule, alphasort);
                if (files_number[current_wnd] < 0)
                {
                    error_msg = strerror(errno);
                    move(window_strings + 3, window_columns+2 - strlen(error_msg)/2);
                    printw("%s", error_msg);
                    refresh();
                    return RETURN_ERR; 
                } 
                update_window(subwnd[current_wnd], namelist[current_wnd], files_number[current_wnd]);
                break; 
            case KEY_LEFT:
                if(current_wnd == first_wnd)
                    break;   

                if(cursore_string > files_number[current_wnd])
                    break;   
                current_file = cursore_string;

                if(check_file_type(current_dir[current_wnd], namelist[current_wnd][current_file]->d_name))
                    break;               
                strcpy(current_dir[first_wnd], current_dir[second_wnd]);       
                update_current_dir(namelist[current_wnd], current_file, current_dir[first_wnd]);
                current_wnd = first_wnd; 

                while (files_number[current_wnd]--) 
                {
                    free(namelist[current_wnd][files_number[current_wnd]]);
                }
                free(namelist[current_wnd]);

                files_number[current_wnd] = scandir(current_dir[current_wnd], &namelist[current_wnd], no_rule, alphasort);
                if (files_number[current_wnd] < 0)
                {
                    error_msg = strerror(errno);
                    move(window_strings + 3, window_columns+2 - strlen(error_msg)/2);
                    printw("%s", error_msg);
                    refresh();
                    return RETURN_ERR; 
                } 
                update_window(subwnd[current_wnd], namelist[current_wnd], files_number[current_wnd]);
                break;
            case _KEY_ENTER:
                if(cursore_string > files_number[current_wnd])
                    break;
                current_file = cursore_string;

                if(check_file_type(current_dir[current_wnd], namelist[current_wnd][current_file]->d_name))
                    break;
                
                update_current_dir(namelist[current_wnd], current_file, current_dir[current_wnd]);

                while (files_number[current_wnd]--) 
                {
                    free(namelist[current_wnd][files_number[current_wnd]]);
                }
                free(namelist[current_wnd]);

                files_number[current_wnd] = scandir(current_dir[current_wnd], &namelist[current_wnd], no_rule, alphasort);
                if (files_number[current_wnd] < 0)
                {
                    error_msg = strerror(errno);
                    move(window_strings + 3, window_columns+2 - strlen(error_msg)/2);
                    printw("%s", error_msg);
                    refresh();
                    return RETURN_ERR; 
                } 
                update_window(subwnd[current_wnd], namelist[current_wnd], files_number[current_wnd]);
                break;                 
            default:
                getyx(stdscr, cursore_string, cursore_column);
                mvwaddch(stdscr, cursore_string, cursore_column-1, ' ');
                refresh();
                move(cursore_string, cursore_column-1);
                break;
        }

    }
    
    for(i = 0; i < all_wnd; i++)
    {
        delwin(subwnd[i]);
        delwin(wnd[i]);
    }
    endwin();                    

    for(i = 0; i < all_wnd; i++)
    {
        while (files_number[i]--) 
        {
            free(namelist[i][files_number[i]]);
        }    
        free(namelist[i]);
    }  

    return 0;
}

 int check_file_type(char *dir, char *file)
 {
    struct stat sb;
    char *file_type_txt = NULL;
    char *file_path = malloc(strlen(dir) + strlen(file) + 2);
    strcpy(file_path, dir);
    strcat(file_path, "/");
    strcat(file_path, file);

    move(window_strings + 3, 0);
    clrtoeol();
    refresh();
    
    if (lstat(file_path, &sb) < 0) {     
        char *error_msg = strerror(errno);
        print_error_msg(error_msg); 
        free(file_path);      
        return RETURN_ERR;
    }

    if((sb.st_mode & S_IROTH) != S_IROTH) {
        print_error_msg(OPEN_ERR_TXT);
        free(file_path);
        return RETURN_ERR;
    }

    switch (sb.st_mode & S_IFMT) 
    {
        case S_IFBLK:  file_type_txt = S_IFBLK_TXT;     break;
        case S_IFCHR:  file_type_txt = S_IFCHR_TXT;     break;
        case S_IFDIR:  free(file_path);                 return RETURN_OK;
        case S_IFIFO:  file_type_txt = S_IFIFO_TXT;     break;
        case S_IFLNK:  free(file_path);                 return RETURN_OK;
        case S_IFREG:  file_type_txt = S_IFREG_TXT;     break;
        case S_IFSOCK: file_type_txt = S_IFSOCK_TXT;    break;
        default:       file_type_txt = S_UNKNOWN_TXT;   break;
    } 

    move(window_strings + 3, window_columns+2 - strlen(file));
    printw("%s %s", file, file_type_txt);
    refresh();
    free(file_path);
    return RETURN_ERR;
 }

void update_current_dir(struct dirent ** namelist, int file, char *dir)
{
    char *str_index;
    if(!strcmp(namelist[file]->d_name, CURRENT_DIR)){
       return;
    }
    else if(!strcmp(namelist[file]->d_name, PREVIOUS_DIR)){
        str_index = rindex(dir, '/');          
        *str_index = '\0';
    }
    else {
        strcat(dir, "/");
        strcat(dir, namelist[file]->d_name);
    }
}

void update_window(WINDOW * wnd, struct dirent ** namelist, int files)
{
    int i;
    werase(wnd);
    wmove(wnd, 0, 0);
    for (i = 0; i < files; i++)
    {
        wprintw(wnd, "%s\n", namelist[i]->d_name);
    }                
    wmove(wnd, 0, 0);
    wrefresh(wnd);
}

void print_error_msg(char *error_msg)
{
    move(window_strings + 3, 0);
    clrtoeol();        
    move(window_strings + 3, window_columns+2 - strlen(error_msg)/2);
    printw("%s", error_msg);
    refresh();
}

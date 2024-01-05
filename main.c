#include <dirent.h>
#include <errno.h>
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_NAME 256
#define TEXT_COLOR 8
#define BG_COLOR 9
#define TEST_COLOR 10
#define TEST_COLOR2 11
#define ENTER_KEY 13
#define MOVE_UP 'k'
#define MOVE_DOWN 'j'
#define QUIT_KEY 'q'
#define MAX_DEPTH 5

#define new_color(index, r, g, b)                                              \
  init_color((index), (r) / 0.255, (g) / 0.255, (b) / 0.255)

void init();
struct dirent **get_file_and_folders(char *path_name);
void print_files_and_folders(struct dirent **file_and_folders);
void respond_to_key(int c, char *cwd_name,
                    struct dirent ***file_and_folders_in_cwd, int *selection);
void preview(struct dirent file_or_folder);
void print_dir(char *path, int indent, int depth);

WINDOW *dir_window;
WINDOW *dir_border_window;
WINDOW *preview_window;
WINDOW *preview_border_window;

int main() {
  init();

  char cwd_name[MAX_PATH_NAME];
  int selection = 0;

  getcwd(cwd_name, MAX_PATH_NAME);
  struct dirent **file_and_folders_in_cwd = get_file_and_folders(cwd_name);
  print_files_and_folders(file_and_folders_in_cwd);
  mvwchgat(dir_window, selection, 4, 1, A_COLOR | A_BOLD, 2, NULL);
  wrefresh(dir_window);

  int c;
  do {
    c = getch();
    respond_to_key(c, cwd_name, &file_and_folders_in_cwd, &selection);
  } while (c != QUIT_KEY);

  refresh();
  endwin();
}

void respond_to_key(int c, char *cwd_name,
                    struct dirent ***file_and_folders_in_cwd, int *selection) {
  switch (c) {
  case MOVE_DOWN: {
    if ((*file_and_folders_in_cwd)[*selection + 1] != NULL) {
      wclear(dir_window);
      print_files_and_folders(*file_and_folders_in_cwd);
      mvwchgat(dir_window, ++(*selection), 4,
               strlen((*file_and_folders_in_cwd)[*selection]->d_name),
               A_COLOR | A_BOLD, 2, NULL);
      wrefresh(dir_window);
      preview(*(*file_and_folders_in_cwd)[*selection]);
    }
    break;
  };
  case MOVE_UP: {
    if (*selection > 0) {
      wclear(dir_window);
      print_files_and_folders(*file_and_folders_in_cwd);
      mvwchgat(dir_window, --(*selection), 4,
               strlen((*file_and_folders_in_cwd)[*selection]->d_name),
               A_COLOR | A_BOLD, 2, NULL);
      wrefresh(dir_window);
      preview(*(*file_and_folders_in_cwd)[*selection]);
    }
    break;
  };
  case ENTER_KEY: {
    if ((*file_and_folders_in_cwd)[*selection]->d_type == DT_DIR) {
      wclear(dir_window);
      chdir((*file_and_folders_in_cwd)[*selection]->d_name);
      getcwd(cwd_name, MAX_PATH_NAME);
      *file_and_folders_in_cwd = get_file_and_folders(cwd_name);
      print_files_and_folders(*file_and_folders_in_cwd);
      mvwchgat(dir_window, (*selection = 0), 4, 1, A_COLOR | A_BOLD, 2, NULL);
      wrefresh(dir_window);
    }
    break;
  }
  }
}

void preview(struct dirent file_or_folder) {
  switch (file_or_folder.d_type) {
  case DT_REG: {
    wclear(preview_window);
    char *content;
    FILE *f;
    f = fopen(file_or_folder.d_name, "r");
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = malloc(sizeof(char) * (length + 1));
    fread(content, sizeof(char), length, f);
    content[length] = '\0';
    wprintw(preview_window, "%s", content);
    wrefresh(preview_window);
    fclose(f);
    free(content);
    break;
  }
  case DT_DIR: {
    wclear(preview_window);
    print_dir(file_or_folder.d_name, 0, 0);
    wrefresh(preview_window);
  }
  }
}

void print_files_and_folders(struct dirent **file_and_folders) {
  attron(COLOR_PAIR(1));
  struct dirent *current;
  wmove(dir_window, 0, 0);
  for (int j = 0; (current = file_and_folders[j]) != NULL; j++) {
    switch (current->d_type) {
    case DT_DIR: {
      mvwprintw(dir_window, j, 2, " %s", current->d_name);
      break;
    }
    case DT_REG: {
      mvwprintw(dir_window, j, 2, " %s", current->d_name);

      break;
    }
    }
  }
  wrefresh(dir_window);
}

struct dirent **get_file_and_folders(char *path_name) {
  struct dirent **file_and_folders = malloc(sizeof(struct dirent *) * 256);
  DIR *cwd = opendir(path_name);
  if (cwd == NULL) {
    printw("%s\n", strerror(errno));
    return NULL;
  }
  struct dirent *dp;
  int i = 0;
  for (; ((dp = readdir(cwd)) != NULL); i++) {
    file_and_folders[i] = dp;
  }
  file_and_folders[i + 1] = NULL;
  return file_and_folders;
}

void print_dir(char *path, int indent, int depth) {
  DIR *cwd = opendir(path);
  if (cwd == NULL) {
    wprintw(preview_window, "%s\n", strerror(errno));
    return;
  }

  if (depth == MAX_DEPTH) {
    closedir(cwd);
    return;
  }

  struct dirent *dp;
  for (; ((dp = readdir(cwd)) != NULL);) {

    if (strcmp(".", dp->d_name) != 0 && strcmp("..", dp->d_name) != 0) {
      // │
      wprintw(preview_window, "│");
      for (int i = 0; i < indent / 4; i++) {
        for (int j = 0; j < 4; j++) {
          wprintw(preview_window, " ");
        }
        wprintw(preview_window, "│");
      }
    }
    switch (dp->d_type) {
    case DT_DIR: {
      if (dp->d_name[0] != '.') {
        char new_path[1024];
        strcpy(new_path, path);
        strcat(new_path, "/");
        strcat(new_path, dp->d_name);
        wprintw(preview_window, "    └──%s\n", dp->d_name);
        print_dir(new_path, indent + 4, depth + 1);
        break;
      }
    }
    case DT_REG: {

      if (strcmp(".", dp->d_name) != 0 && strcmp("..", dp->d_name) != 0) {
        wprintw(preview_window, "    └──%s\n", dp->d_name);
        break;
      }
    }
    }
  }
  closedir(cwd);
}

void init() {
  setlocale(LC_ALL, "");
  initscr();
  start_color();
  raw();
  nonl();
  keypad(stdscr, TRUE);
  noecho();
  refresh();

  int win_height, win_width;
  getmaxyx(stdscr, win_height, win_width);

  preview_window =
      newwin(win_height - 2, win_width / 2 - 2, 1, win_width / 2 + 1);
  dir_window = newwin(win_height - 2, win_width / 2 - 2, 1, 1);
  preview_border_window = newwin(win_height, win_width / 2, 0, win_width / 2);
  dir_border_window = newwin(win_height, win_width / 2, 0, 0);

  new_color(TEXT_COLOR, 0, 166, 251);
  new_color(BG_COLOR, 5, 25, 35);
  new_color(TEST_COLOR, 0, 0, 1000);
  new_color(TEST_COLOR2, 0, 1000, 0);
  init_pair(1, TEXT_COLOR, BG_COLOR);
  init_pair(2, BG_COLOR, TEXT_COLOR);

  wbkgd(dir_window, COLOR_PAIR(1));
  wbkgd(dir_border_window, COLOR_PAIR(1));
  wbkgd(preview_window, COLOR_PAIR(1));
  wbkgd(preview_border_window, COLOR_PAIR(1));

  box(preview_border_window, 0, 0);
  box(dir_border_window, 0, 0);

  wrefresh(dir_border_window);
  wrefresh(dir_window);
  wrefresh(preview_window);
  wrefresh(preview_border_window);
  curs_set(0);
}

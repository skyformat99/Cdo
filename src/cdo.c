#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "cdo.h"

typedef enum {
  NORMAL = 0,
  SEARCH = 1,
  NEW = 2,
} Mode;

typedef struct {
  int completed;
  char *text;
} Todo;

typedef struct {
  int numTodos;
  int numFilteredTodos;
  int focusedIdx;
  Mode mode;
  char *query;
  Todo *newTodo;
  Todo **todos;
  int *filteredTodos;
} State;

State *_state = NULL;

Todo *createTodo () {
  Todo *todo = malloc(sizeof(Todo));
  todo->completed = 0;
  todo->text = NULL;
  return todo;
}

State *createState () {
  State *state = malloc(sizeof(State));
  state->numTodos = 0;
  state->numFilteredTodos = 0;
  state->focusedIdx = 0;
  state->mode = NORMAL;
  state->todos = NULL;
  state->filteredTodos = NULL;
  state->newTodo = NULL;
  state->query = NULL;
  return state;
}

State *initialize () {
  initscr();
  noecho();
  _state = createState();
  return _state;
}

void destroyTodo (Todo *todo) {
  if (todo->text != NULL)
    free(todo->text);
  free(todo);
}

void destroyState (State *state) {
  if (state->newTodo != NULL)
    destroyTodo(state->newTodo);
  if (state->numTodos > 0)
    for (int i = 0; i < state->numTodos; i ++)
      destroyTodo(state->todos[i]);
  if (state->filteredTodos != NULL)
    free(state->filteredTodos);
  free(state->todos);
  free(state);
}

void destroy (State *state) {
  destroyState(state);
  endwin();
}

int matchesQuery (State *state, Todo *todo) {
  if (state->query == NULL || strlen(state->query) <= 0)
    return 1;
  else if (strstr(todo->text, state->query) != NULL)
    return 1;
  else
    return 0;
}

void filterTodos (State *state) {
  char *query = state->query;
  if (query == NULL || strlen(query) <= 0) {
    state->numFilteredTodos = state->numTodos;
    if (state->filteredTodos == NULL) {
      state->filteredTodos = malloc(sizeof(int) * state->numTodos);
    } else {
      state->filteredTodos = realloc(state->filteredTodos, sizeof(int) * state->numTodos);
    }
    for (int i = 0; i < state->numTodos; i ++)
      state->filteredTodos[i] = i;
  } else {
    state->numFilteredTodos = 0;
    free(state->filteredTodos);
    state->filteredTodos = NULL;
    for (int i = 0; i < state->numTodos; i ++) {
      if (matchesQuery(state, state->todos[i])) {
        state->numFilteredTodos ++;
        if (state->filteredTodos == NULL) {
          state->filteredTodos = malloc(sizeof(int));
          state->filteredTodos[0] = i;
        } else {
          state->filteredTodos = realloc(state->filteredTodos, sizeof(int) * state->numFilteredTodos);
          state->filteredTodos[state->numFilteredTodos - 1] = i;
        }
      }
    }
  }
  state->focusedIdx = 0;
}

void addQueryChar (State *state, char c) {
  if (state->query == NULL) {
    state->query = malloc(2 * sizeof(char));
    state->query[0] = c;
    state->query[1] = '\0';
  } else {
    int text_len = strlen(state->query);
    state->query = realloc(state->query,
        sizeof(char) * text_len + 2);
    state->query[text_len] = c;
    state->query[text_len + 1] = '\0';
  }
  filterTodos(state);
}

void deleteQueryChar (State *state) {
  if (state->query != NULL) {
    char *text = state->query;
    int text_len = strlen(text);
    if (text_len <= 0) return;
    text = realloc(text, sizeof(char) * text_len);
    text[text_len - 1] = '\0';
  }
  filterTodos(state);
}

void addNewTodoChar (State *state, char c) {
  if (state->newTodo == NULL)
    state->newTodo = createTodo();
  if (state->newTodo->text == NULL) {
    state->newTodo->text = malloc(2 * sizeof(char));
    state->newTodo->text[0] = c;
    state->newTodo->text[1] = '\0';
  } else {
    int text_len = strlen(state->newTodo->text);
    state->newTodo->text = realloc(state->newTodo->text,
        sizeof(char) * text_len + 2);
    state->newTodo->text[text_len] = c;
    state->newTodo->text[text_len + 1] = '\0';
  }
}

void deleteNewTodoChar (State *state) {
  if (state->newTodo != NULL && state->newTodo->text != NULL) {
    char *text = state->newTodo->text;
    int text_len = strlen(text);
    if (text_len <= 0) return;
    text = realloc(text, sizeof(char) * text_len);
    text[text_len - 1] = '\0';
  }
}

void addTodo (State *state) {
  Todo *newTodo = state->newTodo;
  if (newTodo != NULL) {
    if (state->todos == NULL) {
      state->numTodos = 1;
      state->todos = malloc(sizeof(void*));
      state->todos[0] = newTodo;
      if (!matchesQuery(state, newTodo)) return;
      state->numFilteredTodos = 1;
      state->filteredTodos = malloc(sizeof(int));
      state->filteredTodos[0] = 0;
    } else {
      state->numTodos ++;
      state->todos = realloc(state->todos, state->numTodos * sizeof(void*));
      state->todos[state->numTodos - 1] = newTodo;
      if (!matchesQuery(state, newTodo)) return;
      state->numFilteredTodos ++;
      state->filteredTodos = realloc(state->filteredTodos, state->numFilteredTodos * sizeof(int));
      state->filteredTodos[state->numFilteredTodos - 1] = state->numTodos - 1;
    }
    state->newTodo = NULL;
  }
}

void focusDown (State *state) {
  int focus = state->focusedIdx;
  if (focus <= 0)
    state->focusedIdx = 0;
  else
    state->focusedIdx--;
}

void focusUp (State *state) {
  int max = state->numFilteredTodos - 1;
  int focus = state->focusedIdx;
  if (focus >= max)
    state->focusedIdx = max;
  else
    state->focusedIdx++;
}

void selectTodo (State *state) {
  if (state->numFilteredTodos <= 0) return;
  int todoIdx = state->filteredTodos[state->focusedIdx];
  Todo *todo = state->todos[todoIdx];
  if (todo->completed == 0)
    todo->completed = 1;
  else
    todo->completed = 0;
}

void modify_normal (State *state, char c) {
  switch (c) {
    case '/':
      state->mode = SEARCH;
      break;
    case 'n':
      state->mode = NEW;
      break;
    case 'j':
      focusUp(state);
      break;
    case 'k':
      focusDown(state);
      break;
    case 13:
      selectTodo(state);
      break;
    case 10:
      selectTodo(state);
      break;
    default:
      break;
  }
}

void modify_search (State *state, char c) {
  if (c >= 32 && c <= 126)
    addQueryChar(state, c);
  else if (c == 27)
    state->mode = NORMAL;
  else if (c == 127)
    deleteQueryChar(state);
  else if (c == 13 || c == 10)
    state->mode = NORMAL;
}

void modify_new (State *state, char c) {
  if (c >= 32 && c <= 126)
    addNewTodoChar(state, c);
  else if (c == 27)
    state->mode = NORMAL;
  else if (c == 127)
    deleteNewTodoChar(state);
  else if (c == 13 || c == 10)
    addTodo(state);
}

void modify (State *state, char c) {
  switch (state->mode) {
    case NORMAL:
      modify_normal(state, c);
      break;
    case SEARCH:
      modify_search(state, c);
      break;
    case NEW:
      modify_new(state, c);
      break;
  }
}

void render (State *state) {
  clear();
  int cursorx = 0;
  if (state->mode == NORMAL) {
    curs_set(0);
    printw("%d todo(s). ", state->numTodos);
    if (state->query != NULL && strlen(state->query) > 0)
      printw("Searching: '%s'\n", state->query);
    else
      printw("\n");
    printw("Press 'n' to create, '/' to search, 'Ctrl-c' to exit.\n");
    printw("-----------------------------------------------------\n");
  } else if (state->mode == SEARCH) {
    curs_set(1);
    if (state->query != NULL) {
      printw("Search (press ESC to cancel)\n");
      printw("> %s\n", state->query);
      printw("-----------------------------------------------------\n");
      cursorx = 2 + strlen(state->query);
    } else {
      printw("Search (press ESC to cancel)\n");
      printw("> \n");
      printw("-----------------------------------------------------\n");
      cursorx = 2;
    }
  } else if (state->mode == NEW) {
    curs_set(1);
    if (state->newTodo != NULL) {
      printw("New (press ESC to cancel)\n");
      printw("> %s\n", state->newTodo->text);
      printw("-----------------------------------------------------\n");
      cursorx = 2 + strlen(state->newTodo->text);
    } else {
      printw("New (press ESC to cancel)\n");
      printw("> \n");
      printw("-----------------------------------------------------\n");
      cursorx = 2;
    }
  }
  for (int i = 0; i < state->numFilteredTodos; i ++) {
    int todoIdx = state->filteredTodos[i];
    if (i == state->focusedIdx && state->mode == NORMAL)
      printw(">");
    else
      printw(" ");
    if (state->todos[todoIdx]->completed)
      printw(" -");
    else
      printw(" +");
    printw(" %s\n", state->todos[todoIdx]->text);
  }
  move(1, cursorx);
}

void loop (State *state) {
  render(state);
  modify(state, getch());
  loop(state);
}

void start (State *state) {
  loop(state);
  destroy(state);
}

void onSIGINT (int sigint) {
  destroy(_state);
  exit(0);
}

int main () {
  signal(SIGINT, onSIGINT);
  State *state = initialize();
  start(state);
  return 0;
}

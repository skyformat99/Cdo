#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "cdo.h"

typedef struct {
  char *text;
} Todo;

typedef struct {
  int numTodos;
  Todo *newTodo;
  Todo **todos;
} State;

State *_state = NULL;

Todo *createTodo () {
  Todo *todo = malloc(sizeof(Todo));
  todo->text = NULL;
  return todo;
}

State *createState () {
  State *state = malloc(sizeof(State));
  state->numTodos = 0;
  state->todos = NULL;
  state->newTodo = NULL;
  return state;
}

State *initialize () {
  initscr();
  noecho();
  curs_set(0);
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
  free(state->todos);
  free(state);
}

void destroy (State *state) {
  destroyState(state);
  free(state);
  curs_set(1);
  endwin();
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
    } else {
      state->numTodos ++;
      state->todos = realloc(state->todos, state->numTodos * sizeof(void*));
      state->todos[state->numTodos - 1] = newTodo;
    }
    state->newTodo = NULL;
  }
}

void modify (State *state, char c) {
  if (c >= 32 && c <= 126)
    addNewTodoChar(state, c);
  else if (c == 127)
    deleteNewTodoChar(state);
  else if (c == 13 || c == 10)
    addTodo(state);
}

void render (State *state) {
  clear();
  if (state->newTodo != NULL)
    printw("New: %s\n", state->newTodo->text);
  else
    printw("New: \n");
  for (int i = 0; i < state->numTodos; i ++)
    printw("%s\n", state->todos[i]->text);
}

int loop (State *state) {
  render(state);
  modify(state, getch());
  return 1;
}

void onSIGINT (int sigint) {
  destroy(_state);
  exit(0);
}

int main () {
  signal(SIGINT, onSIGINT);
  State *state = initialize();
  while(loop(state));
  destroy(state);
  return 0;
}

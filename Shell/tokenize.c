#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#define MAX_INPUT_LENGTH 255

typedef enum {
    START_STATE,
    TOKEN_STATE,
    QUOTE_STATE,
    SPECIAL_CHAR_STATE,
    END_STATE
} TokenizerState;

typedef struct {
    char *token;
    int length;
} Token;

TokenizerState tokenizer_state = START_STATE;
Token tokens[MAX_INPUT_LENGTH];
int num_tokens = 0;

void add_token(char *start, int length) {
    tokens[num_tokens].token = strndup(start, length);
    tokens[num_tokens].length = length;
    num_tokens++;
}

void tokenize(char *input) {
    int i = 0;
    int token_start = 0;

    while (input[i] != '\0') {
        switch (tokenizer_state) {
            case START_STATE:
                if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n') {
                    // Ignore whitespace
                } else if (input[i] == '"') {
                    tokenizer_state = QUOTE_STATE;
                    token_start = i + 1;
                } else if (input[i] == '<' || input[i] == '>' || input[i] == ';' || input[i] == '|') {
                    add_token(&input[i], 1);
                } else {
                    tokenizer_state = TOKEN_STATE;
                    token_start = i;
                }
                break;

            case TOKEN_STATE:
                if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' ||
                    input[i] == '"' || input[i] == '<' || input[i] == '>' ||
                    input[i] == ';' || input[i] == '|') {
                    add_token(&input[token_start], i - token_start);
                    tokenizer_state = START_STATE;
                    i--; // Re-process the current character in the new state
                }
                break;

            case QUOTE_STATE:
                if (input[i] == '"') {
                    add_token(&input[token_start], i - token_start);
                    tokenizer_state = START_STATE;
                }
                break;

            case SPECIAL_CHAR_STATE:
                // Handle special characters here if needed
                break;

            case END_STATE:
                // Do nothing in the end state
                break;
        }
        i++;
    }

    // Check if there is a pending token at the end
    if (tokenizer_state == TOKEN_STATE || tokenizer_state == QUOTE_STATE) {
        add_token(&input[token_start], i - token_start);
    }
}

void print_tokens() {
    for (int i = 0; i < num_tokens; i++) {
        printf("%.*s\n", tokens[i].length, tokens[i].token);
        free(tokens[i].token);
    }
}

int main() {
char input[MAX_INPUT_LENGTH];
  
  if (isatty(fileno(stdin))) {
    printf("Enter a string to tokenize: ");
  }
  
  if (fgets(input, sizeof(input), stdin) == NULL) {
    return 1;
  }

  // Remove the newline character if present
  int len = strlen(input);
  if (input[len - 1] == '\n') {
    input[len - 1] = '\0';
  }

  tokenize(input);
  print_tokens();

  return 0;
}

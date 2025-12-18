#include "shell.h"

// Global parser state
ParserState parser_state;

// Initialize parser
void init_parser(void) {
    parser_state.current_token_index = 0;
}

// Get current token
Token current_token(void) {
    if (parser_state.current_token_index < token_count) {
        return tokens[parser_state.current_token_index];
    }
    
    // Return end token if we've reached the end
    Token end_token;
    end_token.type = TOKEN_END;
    strcpy(end_token.value, "");
    return end_token;
}

// Consume current token and move to next
void consume_token(void) {
    if (parser_state.current_token_index < token_count) {
        parser_state.current_token_index++;
    }
}

// Check if current token matches expected type and consume it
bool expect_token(TokenType type) {
    Token token = current_token();
    if (token.type == type) {
        consume_token();
        return true;
    }
    return false;
}

// Check if current token matches type without consuming
bool match_token(TokenType type) {
    return current_token().type == type;
}

// Parse a name token
// name -> r"[^|&><;]+"
int parse_name(void) {
    if (match_token(TOKEN_NAME)) {
        consume_token();
        return 1; // success
    }
    return 0; // failure
}

// Parse input redirection
// input -> < name | <name
int parse_input(void) {
    if (match_token(TOKEN_INPUT)) {
        consume_token();
        
        if (parse_name()) {
            return 1; // success
        } else {
            return 0;
        }
    }
    return 0; // not an input redirection
}

// Parse output redirection
// output -> > name | >name | >> name | >>name
int parse_output(void) {
    if (match_token(TOKEN_OUTPUT) || match_token(TOKEN_DOUBLE_OUTPUT)) {
        consume_token();
        
        if (parse_name()) {
            return 1; // success
        } else {
            return 0;
        }
    }
    return 0; // not an output redirection
}

// Parse atomic command
// atomic -> name (name | input | output)*
int parse_atomic(void) {
    // Must start with a name (command name)
    if (!parse_name()) {
        return 0;
    }
    
    // Parse optional arguments and redirections
    while (1) {
        if (parse_name()) {
            // Found an argument
            continue;
        } else if (parse_input()) {
            // Found input redirection
            continue;
        } else if (parse_output()) {
            // Found output redirection
            continue;
        } else {
            // No more arguments or redirections
            break;
        }
    }
    
    return 1;
}

// Parse command group (pipeline)
// cmd_group -> atomic (\| atomic)*
int parse_cmd_group(void) {
    // Must start with an atomic command
    if (!parse_atomic()) {
        return 0;
    }
    
    // Parse optional pipe chains
    while (match_token(TOKEN_PIPE)) {
        consume_token(); // consume pipe
        
        if (!parse_atomic()) {
            return 0;
        }
    }
    
    return 1;
}

// Parse shell command
// shell_cmd -> cmd_group ((& | ;) cmd_group)* &?
int parse_shell_cmd(void) {
    // Must start with a command group
    if (!parse_cmd_group()) {
        return 0;
    }
    
    // Parse optional command separators and additional command groups
    while (match_token(TOKEN_AMPERSAND) || match_token(TOKEN_SEMICOLON)) {
        TokenType separator = current_token().type;
        consume_token();
        
        // Check if there's another command group or if it's just a trailing &
        if (!match_token(TOKEN_END)) {
            if (!parse_cmd_group()) {
                // If we found a separator but no command group follows,
                // it might be a trailing & which is allowed
                if (separator == TOKEN_AMPERSAND && match_token(TOKEN_END)) {
                    break;
                } else {
                    printf("Error: Expected command after separator\n");
                    return 0;
                }
            }
        } else {
            // Trailing separator (only & is allowed)
            if (separator == TOKEN_AMPERSAND) {
                // Background process
            }
            break;
        }
    }
    
    // Check for optional trailing &
    if (match_token(TOKEN_AMPERSAND)) {
        consume_token();
    }
    
    return 1;
}

// Main parse function
int parse(void) {
    init_parser();
    
    // Handle empty input
    if (token_count == 0 || match_token(TOKEN_END)) {
        return 1;
    }
    
    int result = parse_shell_cmd();
    
    // Check if we consumed all tokens
    if (result && !match_token(TOKEN_END)) {
        printf("Warning: Unexpected tokens at end of input\n");
        // Show remaining tokens
        while (!match_token(TOKEN_END)) {
            Token token = current_token();
            printf("Remaining token: %s\n", token.value);
            consume_token();
        }
    }
    
    return result;
}

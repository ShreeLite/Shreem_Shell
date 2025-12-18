#include "shell.h"

Token tokens[1024];
int token_count=0;
const char*current_input;

//Function to tokenise
void tokenise()
{
    token_count=0;
    const char*p=current_input;

    while(*p!='\0' && token_count<1023)
    {   //skipping whitespace characters
        if(isspace(*p))
        {
            p++;
            continue;
        }
        //identifying tokens
        Token*token=&tokens[token_count];
        if(*p=='|')
        {
            token->type=TOKEN_PIPE;
            strncpy(token->value,p,1);
            token->value[1]='\0';
            p++;
        }
        else if(*p=='<')
        {
            token->type=TOKEN_INPUT;
            strncpy(token->value,p,1);
            token->value[1]='\0';
            p++;
        }
        else if(*p=='>')
        {
            if (*(p + 1) == '>') { 
                token->type = TOKEN_DOUBLE_OUTPUT;
                strncpy(token->value, p, 2);
                token->value[2] = '\0';
                p += 2;
            } else {
                token->type = TOKEN_OUTPUT;
                strncpy(token->value, p, 1);
                token->value[1] = '\0';
                p++;
            }
        }
        else if(*p=='&')
        {
            token->type=TOKEN_AMPERSAND;
            strncpy(token->value,p,1);
            token->value[1]='\0';
            p++;
        }
        else if(*p==';')
        {
            token->type=TOKEN_SEMICOLON;
            strncpy(token->value,p,1);
            token->value[1]='\0';
            p++;
        }
        else{
            token->type=TOKEN_NAME;
            int i=0;
            while(*p!='\0' && !isspace(*p) && !strchr("<|>&;",*p))
            {
                token->value[i++]=*p;
                p++;
            }
            token->value[i]='\0';
        }
        token_count++;
    }
    tokens[token_count].type=TOKEN_END;
    tokens[token_count].value[0]='\0';
}

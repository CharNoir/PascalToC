#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct listelem {
    char data[65];
    struct listelem *next;
} listelem;

enum TYPES {CHARACTER = 1, INTEGER, L_INTEGER, DOUB, FLOATING, BOOLEAN, ARRAY};

typedef struct variables_names{
   listelem* character;
   listelem* integer;
   listelem* l_integer;
   listelem* doub;
   listelem* floating;
   listelem* boolean;
}variables_names;

void dynStrAdd(char** x, int* len, char y);


int isWordEnd(char c);
int funcDetect(FILE *fp_in, FILE *fp_out, char* in_str);
int twoSymb(FILE *fp_out);
void skipSpaces(FILE *fp_in);
void skipUntil(FILE *fp_in, char until);
void copyUntil(FILE *fp_in, FILE *fp_out, char until);
void copyUntil_2(FILE *fp_in, FILE *fp_out, char until_1, char until_2);
void strAddEnd(char* str, char c);

int strInList(listelem **head, char* str);
listelem *pop_front(listelem *head);
variables_names* clearVariables(variables_names* variables);
void traverse(listelem *head);
void push_front(listelem **head, char* str);
void dispose_list(listelem *head);
void variablesNamesInit(variables_names** variable);
void namesPushSorted(variables_names* variable, char* name, char* type);
void namesListPushSorted(variables_names* variable, listelem** head, char* type);
void printVariables(variables_names* variables);
void varListInsert(listelem* var_list, char* type);
int variableType(variables_names* variables, char* word);

int Program(FILE *fp_in, FILE *fp_out);
int MainSearch(FILE *fp_in, FILE *fp_out);
void Var(FILE *fp_in, FILE *fp_out, char* first_variable);
void constAssign(FILE *fp_in, FILE *fp_out, char* first_variable);
int Function(FILE *fp_in, FILE *fp_out);
int Procedure(FILE *fp_in, FILE *fp_out);
void commentRecognition(FILE *fp_in, FILE *fp_out);
void pascalString(FILE *fp_in, FILE *fp_out);
void pascalString2(char* str, int* i, FILE *fp_out);

int Body(FILE *fp_in, FILE *fp_out);

void printIndents(FILE *fp_out);
int printBrackets(FILE *fp_out);

int forLoop(FILE *fp_in, FILE *fp_out);

int in_main = 2; // 1 if in main Var or in main body
int in_var = 0;
int in_const = 0;
int indent = 0;
int one_line = 0;
char c, l_c;

variables_names* global_variables;
variables_names* local_variables;

int main()
{
    FILE *fp_in = fopen("in.txt", "r");
    FILE *fp_out = fopen("out.txt", "w");
    variablesNamesInit(&global_variables);
    variablesNamesInit(&local_variables);
    if (fp_in == NULL)
        return -1;

    // including libraries
    fprintf(fp_out, "#include <stdio.h>\n");
    fprintf(fp_out, "#include <stdlib.h>\n");
    fprintf(fp_out, "#include <string.h>\n");
    fprintf(fp_out, "#include <stdbool.h>\n");
    fprintf(fp_out, "#include <math.h>\n\n");

    // create built-in functions and add them into global_variables list
    fprintf(fp_out, "int pred(int a){return a-1;}\n");
    namesPushSorted(global_variables, "pred", "integer");

    fprintf(fp_out, "int succ(int a){return a+1;}\n");
    namesPushSorted(global_variables, "succ", "integer");

    fprintf(fp_out, "int ord(int a){return a;}\n");
    namesPushSorted(global_variables, "ord", "integer");

    fprintf(fp_out, "char chr(int a){return a;}\n");
    namesPushSorted(global_variables, "chr", "char");

    fprintf(fp_out, "int odd(int a){return a%%2;}\n");
    namesPushSorted(global_variables, "odd", "integer");

    // start Main part
    MainSearch(fp_in, fp_out);
    //close files
    fclose(fp_in);
    fclose(fp_out);
    return 0;
}

/*
    The main part recognize the most important reserved names in Pascal code:
        program, var, procedure, function, begin.
    In each case a needed function is called.
*/
int MainSearch(FILE *fp_in, FILE *fp_out)
{
    char word[65] = "";
    l_c = '\0';
    while(fscanf(fp_in, "%c", &c) == 1)
    {
        if (!isWordEnd(c))
        {
            strAddEnd(word, c);
        }
        else
        {
            printf("%s\n", word);
            commentRecognition(fp_in, fp_out);
            if (!strcasecmp(word, "program"))
            {
                fprintf(fp_out, "\n");
                Program(fp_in, fp_out);
            }
            else if (!strcasecmp(word, "var"))
            {
                in_var = 1;
                in_const = 0;
                Var(fp_in, fp_out, "");
            }
            else if (!strcasecmp(word, "const"))
            {
                in_const = 1;
                in_var = 0;
                constAssign(fp_in, fp_out, word);
            }
            else if (!strcasecmp(word, "procedure"))
            {
                local_variables = clearVariables(local_variables);
                in_main = 0;
                in_var = 0;
                in_const = 0;
                Procedure(fp_in, fp_out);
            }
            else if (!strcasecmp(word, "function"))
            {
                local_variables = clearVariables(local_variables);
                in_main = 0;
                in_var = 0;
                in_const = 0;
                Function(fp_in, fp_out);
            }
            else if (!strcasecmp(word, "begin"))
            {
                in_var = 0;
                in_const = 0;
                if(Body(fp_in, fp_out))
                    return 0;
                in_main = 2;
                local_variables = clearVariables(local_variables);
            }
            else if ( (in_const == 1) && (strcasecmp(word, "")) )
                constAssign(fp_in, fp_out, word);
            else if ( (in_var == 1) && (strcasecmp(word, "")) )
                Var(fp_in, fp_out, word);
            word[0] = '\0';
        }
        l_c = c;
    }
    return 0;
}

// if Var is found
void Var(FILE *fp_in, FILE *fp_out, char* first_variable)
{
    /*
        create passed list
            create non-passed list -> store words until ':'
            recognize the type of a variable -> copy non-passed list to passed list
    */
    listelem* var_list = NULL;
    if (strcasecmp(first_variable, ""))
        push_front(&var_list, first_variable);

    char word[65] = "";
    fscanf(fp_in, "%c", &c);
    while(c != ':')
    {
        commentRecognition(fp_in, fp_out);
        if (!isWordEnd(c))
        {
            strAddEnd(word, c);
        }
        else
        {
            if (strcasecmp(word, ""))
                push_front(&var_list, word);
            word[0] = '\0';
        }
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }
    if (strcasecmp(word, ""))
        push_front(&var_list, word);
    c = '\0';
    word[0] = '\0';
    skipSpaces(fp_in);

    while(!isWordEnd(c))
    {
        strAddEnd(word, c);
        fscanf(fp_in, "%c", &c);
    }
    // data type recognition
    char type[10] = "";

    int arr = 0;
    char* len[65];
    if (!strcasecmp(word, "array"))
    {
        arr = 1;
        skipUntil(fp_in, '.');
        skipUntil(fp_in, '.');
        word[0] = '\0';
        fscanf(fp_in, "%c", &c);
        while (c!=']')
        {
            strAddEnd(word, c);
            fscanf(fp_in, "%c", &c);
        }
        strcpy(len, word);

        while (c!=';')
        {
            if (!isWordEnd(c))
                strAddEnd(word, c);
            else
            {
                if (!strcasecmp(word, "of"))
                    break;
                word[0] = '\0';
            }
            l_c = c;
            fscanf(fp_in, "%c", &c);
        }
        skipSpaces(fp_in);
        word[0] = '\0';
        while(!isWordEnd(c))
        {
            strAddEnd(word, c);
            fscanf(fp_in, "%c", &c);
        }

    }
    if (!strcasecmp(word, "integer") || !strcasecmp(word, "byte") || !strcasecmp(word, "shortint"))
    {
        strcpy(type, "int");
        varListInsert(var_list, word);
    }
    if (!strcasecmp(word, "word") || !strcasecmp(word, "longint"))
    {
        strcpy(type, "long");
        varListInsert(var_list, word);
    }
    if (!strcasecmp(word, "real") || !strcasecmp(word, "double"))
    {
        strcpy(type, "double");
        varListInsert(var_list, word);
    }
    if (!strcasecmp(word, "single"))
    {
        strcpy(type, "float");
        varListInsert(var_list, word);
    }
    if (!strcasecmp(word, "boolean"))
    {
        strcpy(type, "bool");
        varListInsert(var_list, word);
    }
    if (!strcasecmp(word, "char"))
    {
        strcpy(type, "char");
        varListInsert(var_list, word);
    }

    if (arr)
    {
        fprintf(fp_out, "%s %s[%s];", type, var_list->data, len);
        dispose_list(var_list);
        return;
    }
    listelem *p;
    p = var_list;

    if (in_var == 0)
    {
        while (p != NULL)
        {
            fprintf(fp_out, "%s %s", type, p->data);
            if (p->next != NULL)
                fprintf(fp_out, ", ");
            p = p -> next;
        }
    }
    else{
        fprintf(fp_out, "%s ", type);
        while (p != NULL)
        {
            fprintf(fp_out, "%s", p->data);
            if (p -> next != NULL)
                fprintf(fp_out, ", ");
            else
                fprintf(fp_out, ";\n");
            p = p -> next;
        }
    }
    traverse(var_list);
    dispose_list(var_list);
}

void constAssign(FILE *fp_in, FILE *fp_out, char* first_variable)
{
    char word[65] = "";
    char name[65] = "";
    if (!strcasecmp(first_variable, "const"))
        first_variable = "";
    if (!strcmp(first_variable, ""))
    {
        skipSpaces(fp_in);
        while(!isWordEnd(c))
        {
            strAddEnd(name, c);
            fscanf(fp_in, "%c", &c);
        }
    }
    else
        strcpy(name, first_variable);
    if(c != '=')
        skipUntil(fp_in, '=');
    skipSpaces(fp_in);
    enum TYPES type = 0;

    char* str = "";
    int length = 1;
    while(c != ';')
    {
        if (type == 0)
        {
            if (c == '\'')
                type = CHARACTER;
            if (c == '.')
                type = DOUB;
        }
        if (c == '\'')
            printf("STRING\n");
        if (!isWordEnd(c))
        {
            strAddEnd(word, c);
        }
        else
        {
            if (!strcasecmp(word, "true") || !strcasecmp(word, "false"))
                  type = BOOLEAN;
            word[0] = '\0';
        }
        if (c != '\0' )
            dynStrAdd(&str, &length, c);
        fscanf(fp_in, "%c", &c);
    }
    if (type == 0)
        type = L_INTEGER;
    fprintf(fp_out, "const ");
    switch(type)
    {
        case CHARACTER: fprintf(fp_out, "char "); namesPushSorted(global_variables, name, "char"); break;
        case DOUB: fprintf(fp_out, "double "); namesPushSorted(global_variables, name, "double"); break;
        case BOOLEAN: fprintf(fp_out, "bool "); namesPushSorted(global_variables, name, "boolean"); break;
        case L_INTEGER: fprintf(fp_out, "long int "); namesPushSorted(global_variables, name, "longint"); break;
    }
    fprintf(fp_out, "%s = ", name);
    int i = 0;
    while(str[i] != '\0')
    {
        fprintf(fp_out, "%c", str[i]);
        i++;
    }
    fprintf(fp_out, ";\n");

    free(str);
}

int Function(FILE *fp_in, FILE *fp_out)
{
    char word[65] = "";
    char name[65] = "";
    indent = -1;

    while (c != '(')
    {
        if (!isWordEnd(c))
            strAddEnd(word, c);
        else
        {
            if (strcasecmp(word, ""))
                strcpy(name, word);
            word[0] = '\0';
        }
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }
    if (strcasecmp(word, ""))
        strcpy(name, word);
    word[0] = '\0';

    FILE *fp_buf = fopen("buf.txt", "w");
    if (c != '(')
        copyUntil(fp_in, fp_buf, '(');
    fprintf(fp_buf, "(");
    while ((c != ')') ||  (l_c == '*'))
    {
        Var(fp_in, fp_buf, "");
        if ( (c == ')') && (l_c != '*') )
            break;
        skipSpaces(fp_in);
        if (c == ',')
            fprintf(fp_buf, ", ");
    }
    fclose(fp_buf);


    skipUntil(fp_in, ':');
    l_c = c;
    fscanf(fp_in, "%c", &c);
    skipSpaces(fp_in);

    while(!isWordEnd(c))
    {
        strAddEnd(word, c);
        fscanf(fp_in, "%c", &c);
    }

    // data type recognition
    namesPushSorted(global_variables, name, word);

    if (!strcasecmp(word, "integer") || !strcasecmp(word, "byte") || !strcasecmp(word, "shortint"))
        strcpy(word, "int");
    if (!strcasecmp(word, "word") || !strcasecmp(word, "longint"))
        strcpy(word, "long");
    if (!strcasecmp(word, "real") || !strcasecmp(word, "double"))
        strcpy(word, "double");
    if (!strcasecmp(word, "single"))
        strcpy(word, "float");
    if (!strcasecmp(word, "boolean"))
        strcpy(word, "bool");
    if (!strcasecmp(word, "char"))
        strcpy(word, "char");

    fprintf(fp_out, "%s %s", word, name);

    fp_buf = fopen("buf.txt", "r");
    char c_buf = c;
    while(fscanf(fp_buf, "%c", &c) == 1)
        fprintf(fp_out, "%c", c);
    fclose(fp_buf);

    c = c_buf;
    if (c != ';')
        skipUntil(fp_in, ';');
    fprintf(fp_out, "){\n");
    fprintf(fp_out, "%s %s;\n", word, name);
    return 0;
}

int Procedure(FILE *fp_in, FILE *fp_out)
{
    indent = -1;
    fprintf(fp_out, "\nvoid ");
    if (c != '(')
        copyUntil(fp_in, fp_out, '(');
    fprintf(fp_out, "(");
    while ((c != ')') ||  (l_c == '*'))
    {
        Var(fp_in, fp_out, "");
        if ( (c == ')') && (l_c != '*') )
            break;
        skipSpaces(fp_in);
        if (c == ',')
            fprintf(fp_out, ", ");
    }
    skipUntil(fp_in, ';');
    fprintf(fp_out, "){\n");

    return 0;
}

int Body(FILE *fp_in, FILE *fp_out) //returns 1 if it is main
{
    if (in_main == 2)
    {
        fprintf(fp_out, "int main()\n{\n");
        in_main = 1;
    }
    else if (indent != -1)
        fprintf(fp_out, "{\n");
    if (indent == -1)
        indent = 0;

    indent++;
    printIndents(fp_out);
    char word[65] = "";
    fscanf(fp_in, "%c", &c);
    while ( (strcasecmp(word, "end")) && (!((c == ';')&&one_line)) )
    {
        commentRecognition(fp_in, fp_out);
        printBrackets(fp_out);
        if (!isWordEnd(c))
            strAddEnd(word, c);
        else
        {
            switch (funcDetect(fp_in, fp_out, word))
            {
                case 0: fprintf(fp_out, "%s", word); break;
                case 2: return 1;
            }
            if ( (c == ';')&&one_line )
            {
                indent--;
                printIndents(fp_out);
                fprintf(fp_out, "}\n");
                return 0;
            }
            if (strcasecmp(word, "end"))
                twoSymb(fp_out);
            word[0] = '\0';
        }
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }
    indent--;
    printIndents(fp_out);
    if (one_line)
        fprintf(fp_out, ";\n");
    fprintf(fp_out, "}\n");
    printIndents(fp_out);
    one_line = 0;
    if (c == '.')
        return 1;
    else
        c = '\0';
    return 0;
}

int Program(FILE *fp_in, FILE *fp_out)
{
    fprintf(fp_out, "// Program is called \"");
    copyUntil(fp_in, fp_out, ';');
    fprintf(fp_out, "\"\n\n");
    return 0;
}

void commentRecognition(FILE *fp_in, FILE *fp_out)
{
    if (c == '{')
    {
        fprintf(fp_out, "/*");
        copyUntil(fp_in, fp_out, '}');
        fprintf(fp_out, "*/\n");
    }

    if ( (l_c == '(') && (c == '*'))
    {
        fprintf(fp_out, "/*");
        copyUntil_2(fp_in, fp_out, '*', ')');
        fprintf(fp_out, "*/\n");
    }

    if ( (l_c == '/') && (c == '/'))
    {
        fprintf(fp_out, "//");
        copyUntil(fp_in, fp_out, '\n');
        fprintf(fp_out, "\n");
        l_c = '\0';
    }
}

void dynStrAdd(char** x, int* len, char y)
{
    if (strlen(*x) + 2 > *len)
    {
        *len *= 2;
        char* output_string = (char*)malloc( (*len)*sizeof(char) );
        strcpy(output_string, *x);
        free(*x);
        output_string[strlen(output_string) + 1] = '\0';
        output_string[strlen(output_string)] = y;
        *x = output_string;
    }
    else
    {
        (*x)[strlen(*x) + 1] = '\0';
        (*x)[strlen(*x)] = y;
    }
}
int write(FILE *fp_in, FILE *fp_out)
{
    char* str = "";
    int length = 1;
    while (c != ')')
    {
        char word[65] = "";
        enum TYPES type;
        int eqBeg = 1;
        int flag = 0;
        while ( ((c != ',' ) && (c != ')')) || flag )
        {
            if (c == ')')
                flag--;
            fscanf(fp_in, "%c", &c);
            if (c == '(')
                flag++;
            if ( c == '/')
            {
                type = DOUB;
                eqBeg = 0;
            }
            if ( (c == '\'') && (eqBeg == 1) )
            {
                type = ARRAY;
                eqBeg = 0;
            }
            if (!isWordEnd(c))
            {
                strAddEnd(word, c);
            }
            else
            {
                if (!strcasecmp(word, "and") || !strcasecmp(word, "or") || !strcasecmp(word, "xor") || !strcasecmp(word, "not"))
                    type = BOOLEAN;
                if (eqBeg == 1 && strcasecmp(word, ""))
                {
                    if (variableType(local_variables, word))
                    {
                        eqBeg = 0;
                        type = variableType(local_variables, word);
                    }
                    else if (variableType(global_variables, word))
                    {
                        eqBeg = 0;
                        type = variableType(global_variables, word);
                    }
                }
                word[0] = '\0';
            }

            if ( ((c != ',' ) && (c != ')')) || flag )
                if (c != '\0' )
                    dynStrAdd(&str, &length, c);
            l_c = c;
        }
        dynStrAdd(&str, &length, ' ');
        // store 2 char
        char c_buf = c;
        char l_c_buf = l_c;
        switch (type)
        {
            case CHARACTER: fprintf(fp_out, "printf(\"%%c\", "); break;
            case INTEGER: fprintf(fp_out, "printf(\"%%d\", "); break;
            case L_INTEGER: fprintf(fp_out, "printf(\"%%ld\", "); break;
            case DOUB: fprintf(fp_out, "printf(\"%%lf\", "); break;
            case FLOATING: fprintf(fp_out, "printf(\"%%f\", "); break;
            case BOOLEAN: fprintf(fp_out, "printf(\"%%d\", "); break;
            case ARRAY: fprintf(fp_out, "printf(\"%%s\", "); break; // not finished
        }
        l_c = '\0';
        c = str[0];
        for (int i = 1; c != '\0'; i++)
        {
            if (c == '\'')
                pascalString2(str, &i, fp_out);
            if (!isWordEnd(c))
            {
                strAddEnd(word, c);
            }
            else
            {
                switch (funcDetect(fp_in, fp_out, word))
                {
                    case 0: fprintf(fp_out, "%s", word); break;
                    case 2: return 1;
                }
                word[0] = '\0';
            }
            if (c == '(')
                fprintf(fp_out, "(");
            if (c == ')')
                fprintf(fp_out, ")");
            if (strcasecmp(word, "end"))
                twoSymb(fp_out);
            l_c = c;
            c = str[i];
        }
        fprintf(fp_out, ");\n");

        free(str);
        str = "";
        length = 1;
        c = c_buf;
        l_c = l_c_buf;
        if (c == ',')
            c = '\0';
    }
    skipUntil(fp_in, ';');
    if (!one_line)
        c = '\0';
    return 0;
}

void read(FILE *fp_in, FILE *fp_out)
{
    while (c != ')')
    {
        char word[65] = "";
        char word2[65] = "";
        enum TYPES type;
        while ( (c != ',' ) && (c != ')') )
        {
            fscanf(fp_in, "%c", &c);
            if (!isWordEnd(c))
            {
                strAddEnd(word, c);
            }
            else
            {
                if (strcasecmp(word, ""))
                {
                    if (variableType(local_variables, word))
                        type = variableType(local_variables, word);
                    else if (variableType(global_variables, word))
                        type = variableType(global_variables, word);
                    strcpy(word2, word);
                }
                word[0] = '\0';
            }
            l_c = c;
        }

        switch (type)
        {
            case CHARACTER: fprintf(fp_out, "scanf(\"%%c\", "); break;
            case INTEGER: fprintf(fp_out, "scanf(\"%%d\", "); break;
            case L_INTEGER: fprintf(fp_out, "scanf(\"%%ld\", "); break;
            case DOUB: fprintf(fp_out, "scanf(\"%%lf\", "); break;
            case FLOATING: fprintf(fp_out, "scanf(\"%%f\", "); break;
            case BOOLEAN: fprintf(fp_out, "scanf(\"%%d\", "); break;
            default: printf("ARRAY CAN NOT BE READEN\n"); break;
        }
        fprintf(fp_out, "&%s);\n", word2);

        if (c == ',')
            c = '\0';
    }
    skipUntil(fp_in, ';');
    c = '\0';
}

int forLoop(FILE *fp_in, FILE *fp_out)
{
    printf("FOR\n");
    fprintf(fp_out, "for (");
    char word[65] = "";
    char variable[65] = "";
    fscanf(fp_in, "%c", &c);
    while ( (strcasecmp(word, "to")&&strcasecmp(word, "down")) || !isWordEnd(c))
    {
        commentRecognition(fp_in, fp_out);
        if (!isWordEnd(c))
            strAddEnd(word, c);
        else
        {
            if (!strcasecmp(variable, "") )
                if ( variableType(local_variables, word) || variableType(global_variables, word) )
                    strcpy(variable, word);
            switch (funcDetect(fp_in, fp_out, word))
            {
                case 0: fprintf(fp_out, "%s", word); break;
                case 2: return 1;
            }
            if (strcasecmp(word, "end"))
                twoSymb(fp_out);
            word[0] = '\0';
        }
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }
    int type =0;
    if (!strcasecmp(word, "down"))
        type = 1;
    fprintf(fp_out, ";%s", variable);
    switch (type){
        case 0: fprintf(fp_out, " <= "); break;
        case 1: fprintf(fp_out, " >= "); break;}

    word[0] = '\0';
    while ( strcasecmp(word, "do") || !isWordEnd(c) )
    {
        commentRecognition(fp_in, fp_out);
        if (!isWordEnd(c))
            strAddEnd(word, c);
        else
        {
            switch (funcDetect(fp_in, fp_out, word))
            {
                case 0: fprintf(fp_out, "%s", word); break;
                case 2: return 1;
            }
            if (strcasecmp(word, "end"))
                twoSymb(fp_out);
            word[0] = '\0';
        }
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }

    fprintf(fp_out, ";");
    switch (type){
        case 0: fprintf(fp_out, "%s++)", variable); break;
        case 1: fprintf(fp_out, "%s--)", variable); break;}
    word[0] = '\0';

    one_line = 1;
    if (Body(fp_in, fp_out))
        return 1;

    return 0;
}

void pascalString(FILE *fp_in, FILE *fp_out)
{
    fprintf(fp_out, "\""); // start string
    fscanf(fp_in, "%c", &l_c);
    fscanf(fp_in, "%c", &c);
    while ( !((c != '\'') && (l_c == '\'')) )
    {
        if ( (c == '\'') && (l_c == '\'') )
            {
                fprintf(fp_out, "\\\'");
                c = '\0';
            }
        else
            if (l_c != '\0')
                fprintf(fp_out, "%c", l_c);
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }

    fprintf(fp_out, "\""); // end string
}
void pascalString2(char* str, int* i, FILE *fp_out)
{
    if (c == '\'')
    {
        fprintf(fp_out, "\""); // start string
        l_c = str[*i];
        (*i)++;
        c = str[*i];
        (*i)++;
        while ( !((c != '\'') && (l_c == '\'')) )
        {
            if ( (c == '\'') && (l_c == '\'') )
                {
                    fprintf(fp_out, "\\\'");
                    c = '\0';
                }
            else
                if (l_c != '\0')
                    fprintf(fp_out, "%c", l_c);
            l_c = c;
            c = str[*i];
            (*i)++;
        }

        fprintf(fp_out, "\""); // end string
    }
}
// returns 0 if c can be a character of a variable
int isWordEnd(char c)
{
    if ( (c >= 40) && (c <= 47) ) // between '(' and '/' in ASCII table
        return 1;
    if ( (c >= 58) && (c <= 64) ) // between ':' and '@' in ASCII table
        return 1;
    if ( (c >= 91) && (c <= 96) ) // between '[' and ''' in ASCII table
        return 1;
    if ( (c >= 123) && (c <= 125) ) // between '{' and '}' in ASCII table
        return 1;
    switch(c)
    {
        case ' ': return 1; break;
        case '\n': return 1; break;
        case '\t': return 1; break;
        default: return 0; break;
    }
}

int funcDetect(FILE *fp_in, FILE *fp_out, char* in_str)
{
    if (!strcasecmp(in_str, "div"))
    {
        fprintf(fp_out, "/");
        return 1;
    }
    if (!strcasecmp(in_str, "mod"))
    {
        fprintf(fp_out, "%%");
        return 1;
    }
    if (!strcasecmp(in_str, "not"))
    {
        fprintf(fp_out, "!");
        return 1;
    }
    if (!strcasecmp(in_str, "and"))
    {
        fprintf(fp_out, "&");
        return 1;
    }
    if (!strcasecmp(in_str, "or"))
    {
        fprintf(fp_out, "|");
        return 1;
    }
    if (!strcasecmp(in_str, "xor"))
    {
        fprintf(fp_out, "^");
        return 1;
    }
    if (!strcasecmp(in_str, "shl"))
    {
        fprintf(fp_out, "<<");
        return 1;
    }
    if (!strcasecmp(in_str, "shr"))
    {
        fprintf(fp_out, ">>");
        return 1;
    }
    if (!strcasecmp(in_str, "true"))
    {
        fprintf(fp_out, " true ");
        return 1;
    }
    if (!strcasecmp(in_str, "false"))
    {
        fprintf(fp_out, " false ");
        return 1;
    }



    if (!strcasecmp(in_str, "write"))
    {
        if (write(fp_in, fp_out))
            return 2;
        return 1;
    }
    if (!strcasecmp(in_str, "writeln"))
    {
        if (write(fp_in, fp_out))
        {
            printIndents(fp_out);
            fprintf(fp_out, "printf(\"\\n\");\n");
            return 2;
        }
        else
        {
            printIndents(fp_out);
            fprintf(fp_out, "printf(\"\\n\");\n");
            return 1;
        }
    }
    if (!strcasecmp(in_str, "read"))
    {
        read(fp_in, fp_out);
        return 1;
    }
    if (!strcasecmp(in_str, "readln"))
    {
        read(fp_in, fp_out);
        printIndents(fp_out);
        fprintf(fp_out, "printf(\"\\n\");\n");
        return 1;
    }

    if (!strcasecmp(in_str, "begin"))
    {
        one_line = 0;
        return 1;
    }

    // detect words for "if statement"
    if (!strcasecmp(in_str, "if"))
    {
        fprintf(fp_out, "if (");
        return 1;
    }
    if (!strcasecmp(in_str, "then"))
    {
        fprintf(fp_out, ")");
        one_line = 1;
        if (Body(fp_in, fp_out))
            return 2;
        return 1;
    }
    if (!strcasecmp(in_str, "else"))
    {
        fprintf(fp_out, "else");
        one_line = 1;
        if (Body(fp_in, fp_out))
            return 2;
        return 1;
    }

    // detect words for "while loop"
    if (!strcasecmp(in_str, "while"))
    {
        fprintf(fp_out, "while (");
        return 1;
    }
    if (!strcasecmp(in_str, "do"))
    {
        fprintf(fp_out, ")");
        one_line = 1;
        if (Body(fp_in, fp_out))
            return 2;
        return 1;
    }

    if (!strcasecmp(in_str, "for"))
    {
        if (forLoop(fp_in, fp_out))
            return 2;
        return 1;
    }

    return 0;
}

/*
    works with 1 and 2 symbols operators
    returns 1 if last character or (last character and current character) is an operator
*/
int twoSymb(FILE *fp_out)
{
    switch (l_c)
    {
        case ':':   switch (c)
                    {
                        case '=': fprintf(fp_out, "="); c = '\0'; return 1;
                        default: /* VAR PART */break;
                    }
                    break;
        case '=':   fprintf(fp_out, "=="); return 1;
        case '<':   switch (c)
                    {
                        case '>': fprintf(fp_out, "!="); return 1;
                        case '<': fprintf(fp_out, "<<"); return 1;
                        case '=': fprintf(fp_out, "<="); return 1;
                        default: fprintf(fp_out, "<"); return 1;
                    }
                    break;
        case '>':   switch (c)
                    {
                        case '>': fprintf(fp_out, ">>"); return 1;
                        case '=': fprintf(fp_out, ">="); return 1;
                        default: fprintf(fp_out, ">"); return 1;
                    }
                    break;
        default:    switch (c)
                    {
                        case ';': fprintf(fp_out, ";\n"); printIndents(fp_out); return 1;
                        case ',':   fprintf(fp_out, ", "); return 1;
                        case '+':   fprintf(fp_out, "+"); return 1;
                        case '[':   fprintf(fp_out, "["); return 1;
                        case ']':   fprintf(fp_out, " -1]"); return 1;
                        case '-':   fprintf(fp_out, "-"); return 1;
                        case '*':   fprintf(fp_out, "*"); return 1;
                        case '/':   fprintf(fp_out, "/(double)"); return 1;
                        //case '\'': pascalString(fp_in, fp_out); return 1;
                        default: /* VAR PART */break;
                    }
    }
    return 0;
}

void skipSpaces(FILE *fp_in)
{
    while( (fscanf(fp_in, "%c", &c) == 1) && (c == ' ' || c == '\t' || c == '\n') ) {}
}

void skipUntil(FILE *fp_in, char until)
{
    while( (fscanf(fp_in, "%c", &c) == 1) && (c != until) ) {}
}

void copyUntil(FILE *fp_in, FILE *fp_out, char until)
{
    fscanf(fp_in, "%c", &c);
    while( (c != until) )
    {
        fprintf(fp_out, "%c", c);
        fscanf(fp_in, "%c", &c);
    }
}

void copyUntil_2(FILE *fp_in, FILE *fp_out, char until_1, char until_2)
{
    fscanf(fp_in, "%c", &l_c);
    fprintf(fp_out, "%c", l_c);
    fscanf(fp_in, "%c", &c);
    while( (l_c != until_1) && (c != until_2) )
    {
        fprintf(fp_out, "%c", l_c);
        l_c = c;
        fscanf(fp_in, "%c", &c);
    }
}

void strAddEnd(char* str, char c)
{
    if (strlen(str) == 64)
    {
        printf("Can not add char to the string, overflow\n");
        return;
    }
    str[strlen(str) + 1] = '\0';
    str[strlen(str)] = c;
}

int strInList(listelem **head, char* str)
{
    listelem *p;
    p = *head;
    while ((p != NULL) && strcmp(p->data, str))
        p = p -> next;
    if (p == NULL)
        return 0;
    else
        return !strcasecmp(p->data, str);
}

listelem *pop_front(listelem *head)
{
    if (head != NULL) /* not empty */
    {
        listelem *p = head;
        head = head->next;
        free(p);
    }
    return head;
}

variables_names* clearVariables(variables_names* variables)
{
    dispose_list(variables->character);

    dispose_list(variables->integer);

    dispose_list(variables->l_integer);

    dispose_list(variables->doub);

    dispose_list(variables->floating);

    dispose_list(variables->boolean);

    variablesNamesInit(&variables);

    return variables;
}

void traverse(listelem *head)
{
    listelem *p;
    for (p = head; p != NULL; p = p->next)
        printf("%s ", p->data);
    printf("\n");
}

void push_front(listelem **head, char* str)
{
    listelem *p = (listelem*)malloc(sizeof(listelem));
    strcpy(p->data, str);
    p->next = *head;
    *head = p;
}

void dispose_list(listelem *head)
{
    while (head != NULL)
        head = pop_front(head);
}

void variablesNamesInit(variables_names** variable)
{
    *variable = (variables_names*)malloc(sizeof(variables_names));
    (*variable)->character = NULL;
    (*variable)->integer = NULL;
    (*variable)->l_integer = NULL;
    (*variable)->doub = NULL;
    (*variable)->floating = NULL;
    (*variable)->boolean = NULL;
}

void namesPushSorted(variables_names* variable, char* name, char* type)
{
    if ( !strcasecmp(type, "char"))
        {push_front(&variable->character, name); return;}
    if ( !strcasecmp(type, "integer") || !strcasecmp(type, "byte") || !strcasecmp(type, "shortint"))
        {push_front(&variable->integer, name); return;}
    if ( !strcasecmp(type, "word") || !strcasecmp(type, "longint"))
        {push_front(&variable->l_integer, name); return;}
    if ( !strcasecmp(type, "real") || !strcasecmp(type, "double"))
        {push_front(&variable->doub, name); return;}
    if ( !strcasecmp(type, "single"))
        {push_front(&variable->floating, name); return;}
    if ( !strcasecmp(type, "boolean"))
        {push_front(&variable->boolean, name); return;}
}

void namesListPushSorted(variables_names* variable, listelem** head, char* type)
{
    listelem *p;
    p = *head;
    while (p != NULL)
    {
        namesPushSorted(variable, p->data, type);
        p = p -> next;
    }
}

void printVariables (variables_names* variables)
{
    printf("character: ");
    traverse(variables->character);
    printf("integer: ");
    traverse(variables->integer);
    printf("l_integer: ");
    traverse(variables->l_integer);
    printf("doub: ");
    traverse(variables->doub);
    printf("floating: ");
    traverse(variables->floating);
    printf("boolean: ");
    traverse(variables->boolean);
}

void varListInsert(listelem* var_list, char* type)
{
    if (in_main)
    {
        namesListPushSorted(global_variables, &var_list, type);
    }
    else
    {
        namesListPushSorted(local_variables, &var_list, type);
    }
    return;
}

int variableType(variables_names* variables, char* word)
{
    if (strInList(&variables->character, word) == 1)
        return CHARACTER;
    if (strInList(&variables->integer, word) == 1)
        return INTEGER;
    if (strInList(&variables->l_integer, word) == 1)
        return L_INTEGER;
    if (strInList(&variables->doub, word) == 1)
        return DOUB;
    if (strInList(&variables->floating, word) == 1)
        return FLOATING;
    if (strInList(&variables->boolean, word) == 1)
        return BOOLEAN;
    return 0;
}

void printIndents(FILE *fp_out)
{
    for (int i = 0; i < indent; i++)
        fprintf(fp_out, "\t");
}

int printBrackets(FILE *fp_out)
{
    if ( (l_c == '(') && (c != '*'))
            {fprintf(fp_out, "("); return 1;}
    if ( l_c == ')')
        {fprintf(fp_out, ")"); return 1;}
    return 0;
}

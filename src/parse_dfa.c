#include "tm_lib.h"

#define HEADER "/*This file is generated by parse_dfa.c*/\n#include\"tm_lib.h\"\n#include\"delta.h\"\ntransition * delta(int state, char symbol){\ntransition * t = malloc(sizeof(transition));\nswitch(state){\n"
#define MAX_INT_DIGITS 10
#define MAX_TRANSITION_LENGTH MAX_INT_DIGITS+5
#define FORMULA

void strToTransition(char * str, int offset, transition * t);

int normalizeInput(FILE * dest, FILE * src);

bool isInteger(char * str);

int main(int argc, char const *argv[]){
    FILE * input = fopen("input_program", "r");
    FILE * input_clean = fopen("input_clean", "w+");
    FILE * state_list = fopen("state_list", "w");
    FILE * output = fopen("delta.c", "w");
    int i, num_length, errorline = 0, line = 1; //TODO fix errorline
    char c;
    bool errorOccurred = false;

    //Buffers
    char * str_state = malloc(6*sizeof(char));                          //"state"
    char * str_acc = malloc(8*sizeof(char));                            //"ACCEPT\n or REJECT\n"
    char * str_arrow = malloc(3*sizeof(char));                          //"->"
    char * str_transition = malloc(MAX_TRANSITION_LENGTH*sizeof(char)); //"integer,char,l or r\n"
    char * str_state_num = malloc((MAX_INT_DIGITS+1)*sizeof(char));     //integer corresponding to a state
    char * ptr;
    transition * wellformed_transition = malloc(sizeof(transition));

    fprintf(output, HEADER);

    //Remove comments and whitespace
    if(normalizeInput(input_clean, input) == -1)
        errorOccurred = true;

    //Parse user input into c code
    while((c = fgetc(input_clean)) != EOF && !errorOccurred){
        if(c != '\n'){
            fseek(input_clean, -1, SEEK_CUR);
            if(!strcmp(fgets(str_state, 6, input_clean), "state")){ //state found

                c = fgetc(input_clean);
                for(i = 0; i < MAX_INT_DIGITS && isdigit(c); ++i){ //get state number
                    str_state_num[i] = c;
                    c = fgetc(input_clean);
                }
                str_state_num[i] = '\0';


                if(c != EOF && isInteger(str_state_num)){
                    printf("State found %d\n", atoi(str_state_num));
                    fprintf(state_list, "%d\n", atoi(str_state_num));
                    fflush(state_list);
                    fprintf(output, "case %d:\n", atoi(str_state_num));
                }else{
                    errorOccurred = true;
                    errorline = line;
                }

                while(c != EOF && c != '\n'){
                    c = fgetc(input_clean);
                }
                line++;
                c = fgetc(input_clean); //c will now be the input symbol or an empty line
                fprintf(output, "switch(symbol){\n");

                //Read each input line until an empty line is found
                while(c != EOF && c != '\n' && !errorOccurred){
                        fprintf(output, "case '%c':\n", c);
                        //Read arrow and output of the transition
                        if(!strcmp(fgets(str_arrow, 3, input_clean), "->")){
                            str_acc = fgets(str_acc, 8, input_clean);
                            if(!strcmp(str_acc, "ACCEPT\n")){
                                fprintf(output, "t->move = ACCEPT;\n");
                            }else if(!strcmp(str_acc, "REJECT\n")){
                                fprintf(output, "t->move = REJECT;\n");
                            }else{
                                //Must read exactly "integer,char,l or r\n" according to the syntax 
                                fseek(input_clean, -6, SEEK_CUR);
                                c = fgetc(input_clean);
                                ptr = str_transition;
                                num_length = 0;
				                for(i = 0; i < MAX_INT_DIGITS && isdigit(c); ++i){
				                    *ptr = c;
				                    c = fgetc(input_clean);
				                    num_length++;
				                    ptr++;
				                }
				                fseek(input_clean, -1, SEEK_CUR);
								fgets(ptr, 6, input_clean);

				                //Check string syntax
                                strToTransition(str_transition, num_length, wellformed_transition);
                                if(wellformed_transition->move != ERROR){
                                    fprintf(output, "t->state = %d;\n", wellformed_transition->state);
                                    fprintf(output, "t->symbol = '%c';\n", wellformed_transition->symbol);
                                    fprintf(output, "t->move = %d;\n", wellformed_transition->move);
                                }else{
                                    errorOccurred = true;
                                    errorline = line;
                                }
                            }
                        }else{
                            errorOccurred = true;
                            errorline = line;
                        }

                        line++;
                        fprintf(output, "break;\n");
                        c = fgetc(input_clean); //c will now be the input symbol or an empty line
                }
                if(c == '\n')
                    line++;
                fprintf(output, "default:\nt->move = ERROR;\nbreak;\n}\nbreak;\n");
            }else{
                errorOccurred = true;
                errorline = line;
            }
        }else{
            line++;
        }
    }
    fprintf(output, "default:\nt->move = ERROR;\nbreak;\n}\nreturn t;\n}");

    //Deallocate
    free(wellformed_transition);
    free(str_state_num);
    free(str_transition);
    free(str_arrow);
    free(str_acc);
    free(str_state);
    fclose(output);
    fclose(state_list);
    fclose(input_clean);
    fclose(input);
    // system("rm input_clean");

    if(!errorOccurred){
        //Compile the generated delta.c and execute tm
        system("gcc -c delta.c");
        system("gcc -c tm_lib.c");
        system("gcc -D DELTA -D FORMULA -o tm.out tm_lib.o delta.o tm.c");
        execve("./tm.out", NULL, NULL);
        printf("Could not execve");
        return EXIT_FAILURE;
    }else{
        if(errorline != 0)
            printf("Error at line %d: parse failed, check input syntax\n", errorline);
        else
            printf("Error: the '#' character is not allowed in input program!\n");
        system("rm delta.c");
        return EXIT_FAILURE;
    }
}

//"integer,char,l or r\n"
void strToTransition(char * str, int offset, transition * t){ //offset is the first integer's length
    char state_num[offset];
    int i;

    if(str[offset] == ',' && isascii(str[offset+1]) && str[offset+2] == ',' && 
      (str[offset+3] == 'l' || str[offset+3] == 'r') && str[offset+4] == '\n'){
    	for(i = 0; i < offset; ++i){
    		state_num[i] = str[i];
    	}
    	state_num[i] = '\0';

    	if(isInteger(state_num)){ //string is well formed
    		t->state = atoi(state_num);
        	t->symbol = str[offset+1];
        	if(str[offset+3] == 'r')
        		t->move = RIGHT;
        	else
        		t->move = LEFT;
    	}else{
    		t->move = ERROR;
    	}
    }else{
    	t->move = ERROR;
    }
}

int normalizeInput(FILE * dest, FILE * src){
	char c;
    while((c = fgetc(src)) != EOF){
        switch(c){
            case '/':
                c = fgetc(src);
                if(c == '/'){
                    while((c = fgetc(src)) != EOF && c != '\n'){}
                }else{
                    fprintf(dest, "/", c);
                }
                if(c != EOF)
                    fseek(src, -1, SEEK_CUR);
            break;

            case ' ':
            break;

            case '#':
                return -1;
            break;

            default:
                fprintf(dest, "%c", c);
            break;  
        }
    }
    fprintf(dest, "\n");
    rewind(dest);

    return 0;
}

//useful because atoi returns 0 if the string is not an integer, but the integer could be 0

bool isInteger(char * str){ 
	int num;
	 
	num = atoi(str);
	if (num == 0 && str[0] != '0')
	   return false;
	else
	   return true;
}
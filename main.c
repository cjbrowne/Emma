#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "include/stack.h"
#include "include/config.h"
#include "include/emma.h"

int main(int argc, char **argv)
{
  assert(HEAPSIZE < MAXSIZE_FILE); // otherwise config.h is screwed
	int i=0,rv=0;
  int first_byte=0,second_byte=0;
  FILE* infile = NULL;
	ramword_t *inputbuffer = NULL;
  stack_t* stack = NULL;
  ramaddr_t* heap = NULL;
  cpu_t* cpu = NULL;
	switch(argc)
	{
		case 0:
			printf("Unsuitable environment (argc==0)\n");
			rv = -1;
		case 1:
			printf("Usage: emma input\n");
			rv = -1;
		case 2:
			if((infile = fopen(argv[1],"r")) == NULL)
			{
				printf("[error] could not open input file, check permissions.\n");
				rv = -1;
			}
			break;
		default:
			printf("Usage: emma input\n");
			rv = -1;
	}
  // if we're successful, get on with the business of the day
  if(rv==0)
  {
    // initialize the CPU
    cpu = malloc(sizeof(cpu_t));
    if(cpu == NULL) return -1; // error
    memset(cpu,0,sizeof(cpu_t));
    if((inputbuffer = malloc(MAXSIZE_FILE * sizeof(*inputbuffer)))==NULL) return -1;
    stack = st_create(STACKSIZE);
    #ifdef EMMA_DEBUG
    fprintf(stderr,"HEAPSIZE: %d\n",HEAPSIZE);
    #endif
    heap = heap_init(HEAPSIZE);
    
    // read the file into memory (more complex than it first seemed)
    while(((first_byte = getc(infile)) != EOF)&&((second_byte = getc(infile)) != EOF)) {
      inputbuffer[i] = (first_byte<<8) | second_byte;
      i++;
      if(i>=MAXSIZE_FILE) break; // this fixes the cause of a segfault
    }
    
    #ifdef EMMA_DEBUG
    int j=0;
    printf("Loaded program: ");
    while(j<i)
    {
      printf("%.4X ",inputbuffer[j]);
      j++;
    }
    putchar('\n');
    #endif
    
    // warn about program truncation
    if(i > HEAPSIZE) 
    {
      #ifdef EMMA_DEBUG
      printf("Warning: input file \"%s\" has been truncated\n",argv[1]);
      #endif
      #ifdef FATAL_ERRORS
      rv = -1;
      #endif
    }
    
    // move the input buffer onto the heap at offset 0x0000, truncated to HEAPSIZE
    if(!heap_load(heap,inputbuffer,HEAPSIZE))
    {
      #ifdef EMMA_DEBUG
      printf("Error: heap loading failed\n");
      #endif
      #ifdef FATAL_ERRORS
      rv = -1;
      #endif
    }
    
    // the input buffer is no longer necessary, so we should free it
    free(inputbuffer);
    
    // set up the cpu with the stack and the heap
    cpu->stack = stack;
    cpu->pc = heap;
    
    if(rv==0)
    {
      // set the program running, return the state of the CPU at exit
      cpu = emu_run(cpu);
    }
    
    #ifdef EMMA_DEBUG
    if((cpu->flag_reg & FLAG_ERROR)!=0) // this if was previously backwards
    {
      printf("Program halted with error: %.2X\n",cpu->errno);
      core_dump(cpu);
    }
    #endif
    
    // shutdown safely (free any used memory)
    st_free(stack);
    heap_free(heap);
    free(cpu);
    fclose(infile);
  }
	return rv;
}

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include "include/config.h"
#include "include/heap.h"
#include "include/stack.h"
#include "include/emma.h"
#include "include/opcodes.h"

cpu_t* emu_run(cpu_t* cpu)
{
  // if we're given an invalid heap:
  if(cpu->pc == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EINVALID_HEAP;
    return cpu;
  }
  while(cpu->pc->value != OPCODE_HLT)
  {
    #ifdef EMMA_DEBUG
    printf("Current opcode: %.4X ",cpu->pc->value);
    sleep(1);
    #endif
    switch(cpu->pc->value)
    {
      case OPCODE_NOP:
        if(cpu->pc->next == NULL)
        {
          cpu->flag_reg |= FLAG_ERROR;
          cpu->errno = EPCOVERFLOW;
          return cpu;
        }
        #ifdef EMMA_DEBUG
        putchar('\n');
        #endif
        cpu->pc = (ramaddr_t*)cpu->pc->next;
        continue;
      OPCODE_CASE(OPCODE_MOV,emu_mov)
      OPCODE_CASE(OPCODE_JMP,emu_jmp)
      OPCODE_CASE(OPCODE_INT,emu_int)
      OPCODE_CASE(OPCODE_OUT,emu_out)
      OPCODE_CASE(OPCODE_INC,emu_inc)
      OPCODE_CASE(OPCODE_POP,emu_pop)
      OPCODE_CASE(OPCODE_PUSH,emu_push)
      OPCODE_CASE(OPCODE_ADD,emu_add)
      OPCODE_CASE(OPCODE_ADC,emu_add)
      OPCODE_CASE(OPCODE_ANDI,emu_andi)
      OPCODE_CASE(OPCODE_ORI,emu_ori)
      OPCODE_CASE(OPCODE_NORI,emu_nori)
      OPCODE_CASE(OPCODE_NOTI,emu_noti)
      OPCODE_CASE(OPCODE_XORI,emu_xori)
      default:
        if(cpu->pc->next == NULL)
        {
          cpu->flag_reg |= FLAG_ERROR;
          cpu->errno = EPCOVERFLOW;
          return cpu;
        }
        else
        {
          cpu->flag_reg |= FLAG_ERROR;
          cpu->errno = EBADOPCODE;
          return cpu;
        }
        break;
    }
  }
  #ifdef EMMA_DEBUG
  printf("Current opcode: %.4X\n",cpu->pc->value);
  #endif
  return cpu;
}

cpu_t* emu_jmp(cpu_t* cpu)
{
  ramaddr_t* arg;                               // start the leap at arg's location, not at the current PC location
  if((arg = (ramaddr_t*)cpu->pc->next)==NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EBADJMP;
  }
  #ifdef EMMA_DEBUG
  printf("relative jump: %.4X\n",arg->value);
  #endif
  if((cpu->pc = heap_leap(arg,arg->value))==NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EBADJMP;
  }
  return cpu;
}

cpu_t* emu_mov(cpu_t* cpu) 
{
  ramaddr_t* dest;
  ramaddr_t* src;
  if((dest = (ramaddr_t*)cpu->pc->next) != NULL && dest->next != NULL) 
  {
    src = (ramaddr_t*)dest->next;
  }
  else
  {
    cpu->flag_reg |= FLAG_ERROR; // program overflow
    cpu->errno = EPCOVERFLOW;
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("dest: %.4X ",dest->value);
  printf("src: %.4X\n",src->value);
  #endif
  switch(dest->value)
  {
    case ACC:
      cpu->acc = src->value;
      break;
    case REG_B:
      cpu->reg_b = src;
      break;
    case REG_C:
      cpu->reg_c = src;
      break;
    default:
      break;
  }
  // at the end of ANY opcode, the PC should be updated, but this one is a little more comple
  cpu->pc = src; // start at arg2
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_int(cpu_t* cpu)
{
  ramaddr_t* arg;
  if((arg = (ramaddr_t*)cpu->pc->next)==NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    return cpu;
  }
  else
  {
    #ifdef EMMA_DEBUG
    printf("int: %.4X\n",arg->value);
    #endif
    switch(arg->value)
    {
      case 0x00:
        // force the cpu into debug mode
        cpu->flag_reg |= FLAG_ERROR;
        cpu->errno = EDEBUGMODE;
        break;
    }
  }
  return cpu;
}

cpu_t* emu_out(cpu_t* cpu)
{
  ramaddr_t* port;
  ramaddr_t* from;
  ramword_t value;
  if((port = (ramaddr_t*)cpu->pc->next)==NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("port: %.4X ",port->value);
  #endif
  if((from = (ramaddr_t*)port->next)==NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("from: %.4X\n",from->value);
  #endif
  switch(from->value)
  {
    case ACC:
      value = cpu->acc;
      break;
    case REG_B:
      value = cpu->reg_b->value;
      break;
    case REG_C:
      value = cpu->reg_c->value;
      break;
  }
  switch(port->value)
  {
    case OUT_CONSOLE:
      printf("%.4X\n",value);
      break;
  }
  cpu->pc = from; // start at second argument
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_inc(cpu_t* cpu)
{
  cpu->acc++;
  #ifdef EMMA_DEBUG
  putchar('\n');
  #endif
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_push(cpu_t* cpu)
{
  st_push(cpu->stack,cpu->acc);
  if(cpu->stack->overflow > 0)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = ESTACKOVERFLOW;
  }
  #ifdef EMMA_DEBUG
  putchar('\n');
  #endif
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_pop(cpu_t* cpu)
{
  int popval = st_pop(cpu->stack);
  if(popval < 0)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = ESTACKUNDERFLOW;
  }
  else
  {
    cpu->acc = popval;
  }
  #ifdef EMMA_DEBUG
  putchar('\n');
  #endif
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_add(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  switch(arg->value)
  {
    case 0x00:
      cpu->acc += cpu->reg_b->value;
      break;
    case 0x01:
      cpu->acc += cpu->reg_c->value;
      break;
  }
  #ifdef EMMA_DEBUG
  putchar('\n');
  #endif
  CPU_INC_PC
  return cpu;
}

cpu_t* emu_adc(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  int result = cpu->acc;
  switch(arg->value)
  {
    case 0x00:
      result += cpu->reg_b->value;
      break;
    case 0x01:
      result += cpu->reg_c->value;
      break;
  }
  if(result > (~(ramword_t)0))
  {
    cpu->flag_reg |= FLAG_CARRY;
  }
  cpu->acc = (ramword_t)result;
  return cpu;
}

cpu_t* emu_andi(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  cpu->acc &= arg->value;
  CPU_INC_PC
  CPU_INC_PC
  return cpu;
}
cpu_t* emu_ori(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  cpu->acc |= arg->value;
  CPU_INC_PC
  CPU_INC_PC
  return cpu;
}
cpu_t* emu_nori(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  cpu->acc = !(cpu->pc->value | arg->value);
  CPU_INC_PC
  CPU_INC_PC
  return cpu;
}
cpu_t* emu_noti(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  cpu->acc = ~(arg->value);
  CPU_INC_PC
  CPU_INC_PC
  return cpu;
}
cpu_t* emu_xori(cpu_t* cpu)
{
  ramaddr_t* arg = (ramaddr_t*)cpu->pc->next;
  if(arg == NULL)
  {
    cpu->flag_reg |= FLAG_ERROR;
    cpu->errno = EPCOVERFLOW;
    #ifdef EMMA_DEBUG
    putchar('\n');
    #endif
    return cpu;
  }
  #ifdef EMMA_DEBUG
  printf("Arg: %d\n",arg->value);
  #endif
  cpu->acc ^= arg->value;
  CPU_INC_PC
  CPU_INC_PC
  return cpu;
}

void core_dump(cpu_t* cpu)
{
  int i=0;
  printf("=== Core Dump ===\n");
  printf("Stack: ");
  if(cpu->stack->size == 0) printf("{empty}\n");
  while(i < cpu->stack->size)
  {
    printf("{%.4X : %.4X}\n",i,st_pop(cpu->stack));
    i++;
  }
  printf("CPU registers:\n");
  printf("Accumulator: %.4X\n",cpu->acc);
  printf("B Register (data): %.4X\n",cpu->reg_b==NULL?0x00:cpu->reg_b->value);
  printf("C Register (data): %.4X\n",cpu->reg_c==NULL?0x00:cpu->reg_c->value);
  printf("Flags:\n");
  printf("\tCarry: %s\n",(cpu->flag_reg & FLAG_CARRY)!=0?"set":"unset");
  printf("\tSign: %s\n",(cpu->flag_reg & FLAG_SIGN)!=0?"set":"unset");
  printf("\tError: %s\n",(cpu->flag_reg & FLAG_ERROR)!=0?"set":"unset");
}

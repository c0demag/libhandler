/* ----------------------------------------------------------------------------
  Copyright (c) 2016, Microsoft Research, Daan Leijen
  This is free software; you can redistribute it and/or modify it under the
  terms of the Apache License, Version 2.0. A copy of the License can be
  found in the file "license.txt" at the root of this distribution.
-----------------------------------------------------------------------------*/

/*
*UNTESTED*
Code for x32 calling convention: Linux
See: https://en.wikipedia.org/wiki/X32_ABI

jump_buf layout 
   0: rip
   8: rbx
  16: rsp
  24: rbp
  32: r12
  40: r13
  48: r14
  56: r15
  64: fpu control word (16 bits)
  66: unused
  68: sse control word (32 bits)
  72: sizeof jmp_buf
*/



.global _lh_setjmp
.global _lh_longjmp

_lh_setjmp:                 /* rdi: jmp_buf */
  movq    (%rsp), %rax      /* rip: return address is on the stack */
  movq    %rax, 0 (%rdi)    

  leaq    8 (%rsp), %rax    /* rsp - return address */
  movq    %rax, 16 (%rdi)   

  movq    %rbx,  8 (%rdi)   /* save registers */
  movq    %rbp, 24 (%rdi) 
  movq    %r12, 32 (%rdi) 
  movq    %r13, 40 (%rdi) 
  movq    %r14, 48 (%rdi) 
  movq    %r15, 56 (%rdi) 

  fnstcw  64 (%rdi)          /* save fpu control word */
  stmxcsr 68 (%rdi)          /* save sse control word */

  xor     %rax, %rax         /* return 0 */
  ret

_lh_longjmp:                  /* rdi: jmp_buf, esi: arg */
  movq  %rsi, %rax            /* return arg to rax */
  
  movq   8 (%rdi), %rbx       /* restore registers */
  movq  24 (%rdi), %rbp
  movq  32 (%rdi), %r12
  movq  40 (%rdi), %r13
  movq  48 (%rdi), %r14
  movq  56 (%rdi), %r15

  fnclex                      /* clear fpu exception flags */
  fldcw   64 (%rdi)           /* restore fpu control word */
  ldmxcsr 68 (%rdi)           /* restore sse control word */

  testl %eax, %eax            /* longjmp should never return 0 */ 
  jnz   ok
  incl  %eax
ok:
  movq  16 (%rdi), %rsp       /* restore the stack pointer */     
  jmpq *(%rdi)                /* and jump to rip */
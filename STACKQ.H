/* Copyright (c) 1990, 1991, 1992 by Info Tech, Inc.
   All rights reserved.*/


/* StackQ.h -- Stack of Queue of String operations.
 *
 * Written by Charles Engelke
 *            November 15, 1990
 *
 * Updated to be a separate file June 25, 1992.
 */


/* StackQueue declarations.  A StackQueue is a stack of queues of strings.
 * Queue operations (AddString and RemoveString) operate on the queue on
 * the top of the stack.  Stack operations create new queues on top of
 * the existing ones (Push) or remove the queue at the top of the stack
 * (Pop).
 */
typedef struct _StackQueue
   {
   char FAR *Buffer;
   UINT uiTopMark;
   UINT uiHead;
   UINT uiTail;
   } StackQueue;

void InitStackQueue (StackQueue *slQueue);
BOOL bEmptyStackQueue (StackQueue slQueue);
void AddString (StackQueue *slQueue, char *szString);
void RemoveString (StackQueue *slQueue, char *szResult);
void Pop (StackQueue *slQueue);
void Push (StackQueue *slQueue);





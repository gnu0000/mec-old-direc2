/* Copyright (c) 1990, 1991, 1992 by Info Tech, Inc.
   All rights reserved.*/


/* StackQ.c -- Stack of Queue of String operations.
 *
 * Written by Charles Engelke
 *            November 15, 1990
 *
 * Updated to be a separate file June 25, 1992.
 */

#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include "stackq.h"
#include "error.h"


void InitStackQueue (StackQueue *slQueue)
   {
   SEL selQueue;
   if (0 != DosAllocSeg (0, &selQueue, SEG_NONSHARED))
      {
      fprintf (stderr, "Not enough memory to run.\n");
      exit (ERROR_OUT_OF_MEMORY);
      }
   slQueue->Buffer              = (char FAR *)MAKEP (selQueue, 0);
   *(UINT FAR *)slQueue->Buffer = 0;
   slQueue->uiTopMark           = 0;
   slQueue->uiHead              = 6;
   slQueue->uiTail              = 6;
   }




BOOL bEmptyStackQueue (StackQueue slQueue)
   {
   return slQueue.uiHead >= slQueue.uiTail;
   }




void RemoveString (StackQueue *slQueue, char *szResult)
   {
   while ((slQueue->uiHead < slQueue->uiTail) &&
          ('\0' != (*szResult = slQueue->Buffer[slQueue->uiHead])))
      {
      szResult++;
      slQueue->uiHead++;
      }
   slQueue->uiHead++;

   if (slQueue->uiHead >= slQueue->uiTail)
      *szResult = '\0';
   }




void Pop (StackQueue *slQueue)
   {
   if (0 != slQueue->uiTopMark)
      {
      slQueue->uiHead    = *(UINT FAR *)(slQueue->Buffer+slQueue->uiTopMark+2);
      slQueue->uiTail    = *(UINT FAR *)(slQueue->Buffer+slQueue->uiTopMark+4);
      slQueue->uiTopMark = *(UINT FAR *)(slQueue->Buffer+slQueue->uiTopMark+0);
      }
   }




void AddString (StackQueue *slQueue, char *szString)
   {
   while ('\0' != (slQueue->Buffer[slQueue->uiTail] = *szString))
      {
      if (slQueue->uiTail == 0xFFFF)
         {
         fprintf (stderr, "Out of memory!\n");
         exit (ERROR_OUT_OF_MEMORY);
         }

      szString++;
      slQueue->uiTail++;
      }
   slQueue->uiTail++;
   }




void Push (StackQueue *slQueue)
   {
   if (slQueue->uiTail >= 0xFFF0)
      {
      fprintf (stderr, "Not enough memory to create queue.\n");
      exit (ERROR_OUT_OF_MEMORY);
      }

   *(UINT FAR *)(slQueue->Buffer+slQueue->uiTail) = slQueue->uiTopMark;
   slQueue->uiTopMark = slQueue->uiTail;

   *(UINT FAR *)(slQueue->Buffer+slQueue->uiTopMark+2) = slQueue->uiHead;
   *(UINT FAR *)(slQueue->Buffer+slQueue->uiTopMark+4) = slQueue->uiTail;

   slQueue->uiHead = slQueue->uiTail = slQueue->uiTopMark+6;
   }






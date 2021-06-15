/* Copyright (c) 1990, 1991, 1992 by Info Tech, Inc.
   All rights reserved.*/


/* DIRECT.C
 *
 * Written by Charles Engelke
 *            November 15, 1990
 *
 *
 * This command lists all the files in specified directories last modified
 * on specified dates.  The output consists of one line per file, with the
 * file size, last date modified and fully qualified file name
 * (including drive letter).
 *
 *
 * Usage:
 *
 * direct [/cdehknopqrtx? /w=<wildcards> /l=<number> /s=<number>
 *         /b=<date-time> /a=<date-time]  {<dir-name>}
 *
 * All parameters are optional and may be in any order.  Case of letters
 * is not significant.  Single-letter commands (/c, etc.) may be combined
 * or not, so "direct /e /r" and "direct /er" are equivalent.  Hyphens
 * may be substituted for slashes.
 *
 * The single-letter parameters are:
 *    /c -- output in Comma-separated value format.
 *    /d -- indicates that subDirectories should be listed as files.
 *    /e -- search Every hard disk drive, from the root.  In this case,
 *             specified directories are ignored.  Direct will think
 *             that network drives are hard disk drives if their drive
 *             letters are contiguous with the hard disk drive letters.
 *    /h -- lists Hidden and system files in addition to normal files.
 *    /k -- Kills (deletes) matching files.
 *    /n -- count liNes of files found.
 *    /o -- Only directories are listed.
 *    /p -- output Pauses after each screenful, and Q quits the listing.
 *    /q -- output is Quiet, with only file names and no sizes or dates.
 *    /r -- Recursively searches subdirectories.
 *    /t -- Total the number of files and the file sizes.
 *    /T -- Total the number of files and the file sizes, but don't 
 *          print out the file names.
 *    /x -- eXcludes searching temporary subdirectories, those named
 *             TMP, TEMP or TEMPORARY.
 *    /z -- Search along path. Turns off /r and /e
 *    /? -- prints Help.
 *    /??-- prints Condensed Help.
 *
 * The parameters that take values are:
 *    /w=<wildcards> Specifies that only files matching the wildcards
 *                   indicated are to be listed.  Multiple wildcards may
 *                   be included by separated with semicolons, as in
 *                      /w=*.h;*.c
 *                   Wildcards are matched as in UNIX, so that the
 *                   wildcard *x* will match only filenames with an x
 *                   in the name or extension.
 *    /l=<number>    Only files whose size is larger than or equal to
 *                   the number are listed.
 *    /s=<number>    Only files whose size is smaller than or equal to
 *                   the number are listed.
 *    /b=<date-time> Only files last modified on or before the date-time
 *                   are listed.
 *    /a=<date-time> Only files last modified on or after the date-time
 *                   are listed.
 *    <date-time>    Must be specified in the following format: 
 *                      Format                 Example
 *                      ---------------------- ----------------------
 *                      mm-dd-yyyy:hh:mm:ss.s  11-15-1990:15:12:31.5
 *
 *                   The hyphens can be replaced by slashes or commas,
 *                   and embedded spaces are NOT allowed.  The time can
 *                   omitted, in which case it is assumed to be 00:00:00.0.
 *                   If the time is omitted, there should be no colons
 *                   specified.  The year can be omitted, in which case
 *                   the current year will be assumed.  The date can be
 *                   given as TODAY or YESTERDAY.  Case of letters is
 *                   NOT significant.
 */


#define VERSION "1.07 Prerelease"

/*
 * Revision log:
 *
 *
 * 1.00 - First usable release.
 *
 * 1.01 - Commented out code that added a drive letter to file names,
 *        because it was messing up the usage of network names.  Also
 *        added version number to the output.
 *        --- January 23, 1991, Charles Engelke.
 *
 * 1.02 - Added support for multiple file wildcards, separated by a
 *        semicolon, as in "/w=*.c;*.h".  Also modified intro comments
 *        and on-line help to reflect the change.  Also added /? as
 *        a synonym for /h for consistency with DOS 5.0.
 *
 *        Finally, modified commands to all be one-letter, so those
 *        without data could be combined, as in "direct -er".
 *        --- November 2, 1991, Charles Engelke.
 *
 * 1.03 - Added the /t option to total the number of files and the
 *        file sizes.  Changed /h to "Hidden" instead of "Help".
 *        --- December 19, 1991, Charles Engelke.
 *
 * 1.04 - Added the /n option to count the number of lines in each file
 *        processed, and print it after the file name.  Also made 'T'
 *        not echo file names.
 *        --- December 23, 1991, Mark Hampton.
 *
 * 1.05 - Added the /k option to kill (delete) matching files.
 *        --- January 3, 1992, Mark Hampton.
 *
 * 1.06 - Fixed the date parsing so that the year defaults to the
 *        current year, and so that "yesterday" is correct.
 *
 *        Cleaned up program structure, using multiple source files.
 *        --- June 25, 1992, Charles Engelke
 *
 * 1.07 - Added the Search-Along-Path option /z
 *        --- April 21, 1994, Craig Fitzgerald
 *
 *        Added condensed help option /??.  Probably no one will like it.
 *        --- July 25, 1994, Craig Fitzgerald
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <share.h>
#include <conio.h>
#include "stackq.h"
#include "error.h"
#include <GnuReg.h>



/* Use the hideous macro just like printf to enable pausing, BUT, be
 * sure not to assume that PRINTF(x) is a single statement!
 */
#define PRINTF if(bPause && (++usCurrRow)==usRows) Pause(); printf



typedef struct _DateAndTime
   {
   FDATE date;
   FTIME time;
   } DateAndTime;


#define LEDate(aDate, aTime, bDate, bTime) \
 ((((aDate).year  != (bDate).year)  ? ((aDate).year  < (bDate).year)  : \
  (((aDate).month != (bDate).month) ? ((aDate).month < (bDate).month) : \
  (((aDate).day   != (bDate).day)   ? ((aDate).day   < (bDate).day)   : \
  (((aTime).hours != (bTime).hours) ? ((aTime).hours < (bTime).hours) : \
  (((aTime).minutes != (bTime).minutes) ? ((aTime).minutes < (bTime).minutes) : \
   ((aTime).twosecs <= (bTime).twosecs)))))))


void Pause (void);
void SearchDir (char *szDirName);
char *szMakeFileName (char *szResult, char *szDir, char *szName);
char *szCurrentDisk (char *szResult);
char *szParameterValue (char *szParameter);
void PrintFile (FILEFINDBUF *pFileBuf, char *szSearchDir);
void PrintDirectory (FILEFINDBUF *pFileBuf, char *szSearchDir);
void ParseParameter (char *szParameter);
void ParseDateTime (char *szDateTime, DateAndTime *datResult);
BOOL bTempDir (char *szDirName);
void PrintHelp(void);
void PrintShortHelp(void);
BOOL bMatchWildCards (char *szString, char **ppszWildCards);
BOOL bMatchWildCard (char *szString, char *szWildCard);
char *pMakeCSVField (char *pszString, char *pResult);
static void InsertChar (char c, char *pszString);

static ULONG CountLines (char *pszFile);
static void KillFile (FILEFINDBUF *pFileBuf);
static BOOL NextPathEntry (char * pszEntry, char * *ppszPath);

/* Global variables controlling search. */
BOOL        bRecurse         = FALSE;
BOOL        bSearchPath      = FALSE;
BOOL        bEveryHardDisk   = FALSE;
BOOL        bDirectories     = FALSE;
BOOL        bOnlyDirectories = FALSE;
BOOL        bExcludeTemps    = FALSE;
BOOL        bQuiet           = FALSE;
BOOL        bHidden          = FALSE;
BOOL        bCSV             = FALSE;
BOOL        bPause           = FALSE;
BOOL        bTotal           = FALSE;
BOOL        bVeryQuiet       = FALSE;
BOOL        bLineCount       = FALSE;
BOOL        bKill            = FALSE;
BOOL        bCmd             = FALSE;
ULONG       ulMinimum        = 0L;
ULONG       ulMaximum        = 0xFFFFFFFF;
ULONG       ulNumFiles       = 0L;
ULONG       ulFileTotal      = 0L;
ULONG       ulLineTotal      = 0L;
ULONG       ulFileAlloc      = 0L;
DateAndTime Before           = {{31, 15, 127}, {31, 63, 31}};
DateAndTime After            = {{ 0,  0,   0}, { 0,  0,  0}};
char        szWildCard[CCHMAXPATHCOMP];
//////PRX         prxWildCard = NULL;
//char        *ppszWildCards[128];
PRX         prxWildCards[128];
FILEFINDBUF findbuf;
char        szBuffer[CCHMAXPATHCOMP];
char        szCmd[CCHMAXPATHCOMP];
StackQueue slDirs;
USHORT      usRows;
USHORT      usCurrRow;



int main (int argc, char *argv[])
   {
   VIOMODEINFO viomi;
   static char szSearchDir[CCHMAXPATHCOMP];
   int iSearchCount = 0;
   USHORT usPathSize;
   USHORT usDisk;
   ULONG  ulDrives;
   char szNumFiles[14];
   char szLineTotal[14];
   char szFileTotal[14];
   char szFileAlloc[14];
   int i;
   int j;

   fprintf (stderr, "Directory Lister %s\n", VERSION);
   fprintf (stderr,
            "Copyright (c) 1990, 1991 by Info Tech, Inc.  All Rights Reserved.\n\n");

   viomi.cb = sizeof viomi;
   VioGetMode (&viomi, (HVIO) 0);
   usRows = viomi.row;
   usCurrRow = 1;

   szWildCard[0] = '\0';
   InitStackQueue (&slDirs);


   for (i=1; i<argc; i++)
      {
      if (argv[i][0] == '-' || argv[i][0] == '/')
         {
         ParseParameter (argv[i]+1);
         }
      }

   if ('\0' == *szWildCard)
      {
      prxWildCards[0] = NULL;
      }
   else
      {
      j=1;
      for (i=0; szWildCard[i] != '\0'; i++)
         if (';' == szWildCard[i])
            j++;

      if (j > 127)
         fprintf (stderr, "Too many wildcards!  Only first 127 used.\n");

      ppszWildCards[0] = szWildCard;
      j=1;
      for (i=0; szWildCard[i] != '\0'; i++)
         if (';' == szWildCard[i])
            {
            szWildCard[i] = '\0';
            prxWildCards[j] = _regParseRegex (szWildCard+i+1);
            if (RegIsError ())
               {
               printf ("Invalid Regex: %s", RegIsError ());
               exit (1);
               }
            j++;
            }
      ppszWildCards[j] = NULL;
      }


   if (bEveryHardDisk)
      {
      strcpy (szSearchDir, "B:\\");

      DosQCurDisk (&usDisk, &ulDrives);
      ulDrives >>= 2; /* Skip to drive C: */
      while (ulDrives & 1L)
         {
         szSearchDir[0]++;
         ulDrives >>= 1;
         SearchDir (szSearchDir);
         }
      }
   else if (bSearchPath)
      {
      char *pszPath;
      char szEntry [256];

      if (pszPath = getenv ("PATH"))
         while (NextPathEntry (szEntry, &pszPath))
            SearchDir (szEntry);
      }
   else
      {
      for (i=1; i<argc; i++)
         {
         if (argv[i][0] != '-' && argv[i][0] != '/')
            {
            szSearchDir[0] = '\0';
      
            strcat (szSearchDir, argv[i]);
      
            if (':' == szSearchDir[strlen(szSearchDir)-1])
               strcat (szSearchDir, ".");
      
            if (0 == (j = DosQPathInfo (szSearchDir,
                                        FIL_QUERYFULLNAME,
                                        szBuffer,
                                        sizeof(szBuffer), 0L)))
               strcpy (szSearchDir, szBuffer);
      
            SearchDir (szSearchDir);
            iSearchCount++;
            }
         }

      if (0 == iSearchCount)
         {
         strcpy (szSearchDir, szCurrentDisk (szBuffer));
         DosQCurDir (szSearchDir[0]-'A'+1, szBuffer, &usPathSize);
         strcat (szSearchDir, "\\");
         strcat (szSearchDir, szBuffer);

         SearchDir (szSearchDir);
         }
      }

   if (bTotal)
      {
      if (ulNumFiles >=1000000000L)
         sprintf (szNumFiles, "%lu,%03lu,%03lu,%03lu",
                  ulNumFiles/1000000000L,
                  (ulNumFiles%1000000000L)/1000000L,
                  (ulNumFiles%1000000L)/1000L,
                  ulNumFiles%1000L);
      else if (ulNumFiles >= 1000000L)
         sprintf (szNumFiles, "%lu,%03lu,%03lu",
                  ulNumFiles/1000000L,
                  (ulNumFiles%1000000L)/1000L,
                  ulNumFiles%1000L);
      else if (ulNumFiles >= 1000L)
         sprintf (szNumFiles, "%lu,%03lu",
                  ulNumFiles/1000L,
                  ulNumFiles%1000L);
      else
         sprintf (szNumFiles, "%lu",
                  ulNumFiles);

      if (ulLineTotal >=1000000000L)
         sprintf (szLineTotal, "%lu,%03lu,%03lu,%03lu",
                  ulLineTotal/1000000000L,
                  (ulLineTotal%1000000000L)/1000000L,
                  (ulLineTotal%1000000L)/1000L,
                  ulLineTotal%1000L);
      else if (ulLineTotal >= 1000000L)
         sprintf (szLineTotal, "%lu,%03lu,%03lu",
                  ulLineTotal/1000000L,
                  (ulLineTotal%1000000L)/1000L,
                  ulLineTotal%1000L);
      else if (ulLineTotal >= 1000L)
         sprintf (szLineTotal, "%lu,%03lu",
                  ulLineTotal/1000L,
                  ulLineTotal%1000L);
      else
         sprintf (szLineTotal, "%lu",
                  ulLineTotal);

      if (ulFileTotal >=1000000000L)
         sprintf (szFileTotal, "%lu,%03lu,%03lu,%03lu",
                  ulFileTotal/1000000000L,
                  (ulFileTotal%1000000000L)/1000000L,
                  (ulFileTotal%1000000L)/1000L,
                  ulFileTotal%1000L);
      else if (ulFileTotal >= 1000000L)
         sprintf (szFileTotal, "%lu,%03lu,%03lu",
                  ulFileTotal/1000000L,
                  (ulFileTotal%1000000L)/1000L,
                  ulFileTotal%1000L);
      else if (ulFileTotal >= 1000L)
         sprintf (szFileTotal, "%lu,%03lu",
                  ulFileTotal/1000L,
                  ulFileTotal%1000L);
      else
         sprintf (szFileTotal, "%lu",
                  ulFileTotal);

      if (ulFileAlloc >=1000000000L)
         sprintf (szFileAlloc, "%lu,%03lu,%03lu,%03lu",
                  ulFileAlloc/1000000000L,
                  (ulFileAlloc%1000000000L)/1000000L,
                  (ulFileAlloc%1000000L)/1000L,
                  ulFileAlloc%1000L);
      else if (ulFileAlloc >= 1000000L)
         sprintf (szFileAlloc, "%lu,%03lu,%03lu",
                  ulFileAlloc/1000000L,
                  (ulFileAlloc%1000000L)/1000L,
                  ulFileAlloc%1000L);
      else if (ulFileAlloc >= 1000L)
         sprintf (szFileAlloc, "%lu,%03lu",
                  ulFileAlloc/1000L,
                  ulFileAlloc%1000L);
      else
         sprintf (szFileAlloc, "%lu",
                  ulFileAlloc);

      if (bCSV)
         {
         if (bLineCount)
            {
            PRINTF ("Totals,\"%s\",\"%s\",\"%s\",\"%s\"\n",
                    szNumFiles, szFileTotal, szLineTotal, szFileAlloc);
            }
         else
            {
            PRINTF ("Totals,\"%s\",\"%s\",\"%s\"\n",
                    szNumFiles, szFileTotal, szFileAlloc);
            }
         }
      else
         {
         if (bLineCount)
            {
            PRINTF ("\n%13s Files %13s,%13s  (%s bytes allocated)\n",
                    szNumFiles, szFileTotal, szLineTotal, szFileAlloc);
            }
         else
            {
            PRINTF ("\n%13s Files %13s  (%s bytes allocated)\n",
                    szNumFiles, szFileTotal, szFileAlloc);
            }
         }
      }

   return 0;
   }


static BOOL NextPathEntry (char *pszEntry, char **ppszPath)
   {
   char   *psz;
   USHORT uLen;

   if (!*ppszPath || !**ppszPath)
      return FALSE;

   if (psz = strchr (*ppszPath, ';'))
      {
      uLen = psz - *ppszPath;
      strncpy (pszEntry, *ppszPath, uLen);
      pszEntry[uLen] = '\0';
      *ppszPath += uLen + 1;
      }
   else
      {
      strcpy (pszEntry, *ppszPath);
      *ppszPath = NULL;
      }
   return TRUE;
   }



void SearchDir (char *szSearchDir)
   {
   HDIR        hdir;
   USHORT      usSearchCount;
   char        szFileName [CCHMAXPATHCOMP];
   USHORT      usResult;

   usSearchCount = 1;
   hdir = HDIR_CREATE;
   Push (&slDirs);

   if (bHidden)
      usResult = DosFindFirst(szMakeFileName (szFileName, szSearchDir, "*.*"),
                           &hdir, FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                           &findbuf, sizeof(findbuf),
                           &usSearchCount, 0L);
   else
      usResult = DosFindFirst(szMakeFileName (szFileName, szSearchDir, "*.*"),
                           &hdir, FILE_NORMAL | FILE_DIRECTORY,
                           &findbuf, sizeof(findbuf),
                           &usSearchCount, 0L);

   while (0 == usResult && 0 != usSearchCount)
      {
      if (0 == (findbuf.attrFile & FILE_DIRECTORY))
         {
         if (LEDate(findbuf.fdateLastWrite, findbuf.ftimeLastWrite,
                    Before.date, Before.time) &&
             LEDate(After.date, After.time,
                    findbuf.fdateLastWrite, findbuf.ftimeLastWrite)  &&
             ulMinimum <= findbuf.cbFile &&
             ulMaximum >= findbuf.cbFile &&
             (prxWildCards[0] == NULL ||
              bMatchWildCards (findbuf.achName, prxWildCards)) &&
             !bOnlyDirectories)
            PrintFile (&findbuf, szSearchDir);
         }
      else
         {
         if (0 != strcmp (findbuf.achName, ".")  &&
             0 != strcmp (findbuf.achName, ".."))
            {
            AddString (&slDirs, findbuf.achName);

            if ((bDirectories || bOnlyDirectories) &&
                (LEDate(findbuf.fdateLastWrite, findbuf.ftimeLastWrite,
                        Before.date, Before.time) &&
                 LEDate(After.date, After.time,
                        findbuf.fdateLastWrite, findbuf.ftimeLastWrite)  &&
                 (prxWildCards[0] == NULL ||
                  bMatchWildCards (findbuf.achName, prxWildCards))))
               PrintDirectory (&findbuf, szSearchDir);
            }
         }
         ;
      usSearchCount = 1;
      usResult = DosFindNext(hdir, &findbuf, sizeof(findbuf), &usSearchCount);
      }
   DosFindClose (hdir);

   if (bRecurse)
      {
      while (!bEmptyStackQueue (slDirs))
         {
         RemoveString (&slDirs, szBuffer);
         if (!bExcludeTemps || !bTempDir (szBuffer))
            SearchDir (szMakeFileName (szFileName, szSearchDir, szBuffer));
         }
      }
   Pop (&slDirs);
   }



char *szMakeFileName (char *szResult, char *szDir, char *szName)
   {
   int i;

   strcpy (szResult, szDir);

   /* If necessary, add a trailing \. */
   i = strlen(szResult)-1;
   if (szResult[i] != '\\'
    && szResult[i] != '/'
    && szResult[i] != ':')
      strcat (szResult, "\\");

   /* Finally, add the name */
   strcat (szResult, szName);

   return szResult;
   }



char *szCurrentDisk (char *szResult)
   {
   USHORT usDrive;
   ULONG  ulDriveMap;

   strcpy (szResult, "C:");

   if (0 == DosQCurDisk (&usDrive, &ulDriveMap))
      {
      szResult[0] += usDrive;
      szResult[0] -= 3;
      }

   return szResult;
   }



char szBuffer [CCHMAXPATHCOMP];
char szDate [11];
char szTime [9];
char szSize [14];
char szFileName [CCHMAXPATHCOMP];
ULONG ulNumLines;
char szLineCount [14] = "";

void PrintFile (FILEFINDBUF *pFileBuf, char *szSearchDir)
   {
   if (bTotal)
      {
      ulNumFiles++;
      ulFileTotal += pFileBuf->cbFile;
      ulFileAlloc += pFileBuf->cbFileAlloc;
      }

   if (!(bQuiet || bVeryQuiet))
      {
      sprintf (szDate, "%d/%02d/%04d", 
               pFileBuf->fdateLastWrite.month,
               pFileBuf->fdateLastWrite.day,
               pFileBuf->fdateLastWrite.year+1980);

      sprintf (szTime, "%d:%02d:%02d",
               pFileBuf->ftimeLastWrite.hours,
               pFileBuf->ftimeLastWrite.minutes,
               pFileBuf->ftimeLastWrite.twosecs * 2);

      if (pFileBuf->cbFile < 1000L)
         sprintf (szSize, "%lu", pFileBuf->cbFile);
      else if (pFileBuf->cbFile < 1000000L)
         sprintf (szSize, "%lu,%03lu",
                  pFileBuf->cbFile/1000L,
                  pFileBuf->cbFile%1000L);
      else if (pFileBuf->cbFile < 1000000000L)
         sprintf (szSize, "%lu,%03lu,%03lu",
                  pFileBuf->cbFile/1000000L,
                  (pFileBuf->cbFile%1000000L)/1000L,
                  pFileBuf->cbFile%1000L);
      else
         sprintf (szSize, "%lu,%03lu,%03lu,%03lu",
                  pFileBuf->cbFile/1000000000L,
                  (pFileBuf->cbFile%1000000000L)/1000000L,
                  (pFileBuf->cbFile%1000000L)/1000L,
                  pFileBuf->cbFile%1000L);
      }

   szMakeFileName (szFileName, szSearchDir, pFileBuf->achName);

   if (bLineCount)
      {
      ulNumLines = CountLines (szFileName);
      ulLineTotal += ulNumLines;

      if (bCSV)
         sprintf (szLineCount, "%lu,", ulNumLines);
      else if (ulNumLines < 1000L)
         sprintf (szLineCount, "%13lu", ulNumLines);
      else if (ulNumLines < 1000000L)
         sprintf (szLineCount, "%9lu,%03lu",
                  ulNumLines / 1000L,
                  ulNumLines % 1000L);
      else if (ulNumLines < 1000000000L)
         sprintf (szLineCount, "%5lu,%03lu,%03lu",
                  ulNumLines / 1000000L,
                  (ulNumLines % 1000000L) / 1000L,
                  ulNumLines % 1000L);
      else
         sprintf (szLineCount, "%1lu,%03lu,%03lu,%03lu",
                  ulNumLines / 1000000000L,
                  (ulNumLines % 1000000000L) / 1000000L,
                  (ulNumLines % 1000000L) / 1000L,
                  ulNumLines % 1000L);
      }

   if (bQuiet)
      {
      if (bCSV)
         {
         PRINTF ("%s\n",
                 pMakeCSVField (szFileName, szBuffer));
         }
      else
         {
         PRINTF ("%s\n", szFileName);
         }
      }
   else if (!bVeryQuiet)
      {
      if (bCSV)
         {
         PRINTF ("%s,%s,\"%s\",%s%s\n",
                 szDate,
                 szTime,
                 szSize,
                 szLineCount,
                 pMakeCSVField (szFileName, szBuffer));
         }
      else
         {
         PRINTF ("%10s %8s %13s %s  %s\n", 
                 szDate, szTime, szSize, szLineCount, szFileName);
         }
      }

   if (bCmd)
      {
      USHORT uRet;

      sprintf (szBuffer, "%s %s", szCmd, szFileName);

      uRet = system (szBuffer);

printf ("system (\"%s\"); Ret:%d\n", szBuffer, uRet);
      }

   if (bKill)
      KillFile (pFileBuf);

   }




void PrintDirectory (FILEFINDBUF *pFileBuf, char *szSearchDir)
   {
   if (!bQuiet)
      {
      sprintf (szDate, "%d/%02d/%04d", 
               pFileBuf->fdateLastWrite.month,
               pFileBuf->fdateLastWrite.day,
               pFileBuf->fdateLastWrite.year+1980);

      sprintf (szTime, "%d:%02d:%02d",
               pFileBuf->ftimeLastWrite.hours,
               pFileBuf->ftimeLastWrite.minutes,
               pFileBuf->ftimeLastWrite.twosecs * 2);
      }

   szMakeFileName (szFileName, szSearchDir, pFileBuf->achName);

   if (bQuiet)
      {
      if (bCSV)
         {
         PRINTF ("%s\n",
                 pMakeCSVField (szFileName, szBuffer));
         }
      else
         {
         PRINTF ("%s\n", szFileName);
         }
      }
   else
      {
      if (bCSV)
         {
         PRINTF ("%s,%s,\"%s\",%s\n",
                 szDate,
                 szTime,
                 "<Directory>",
                 pMakeCSVField (szFileName, szBuffer));
         }
      else
         {
         PRINTF ("%10s %8s %13s  %s\n",
                 szDate, szTime, "<Directory>", szFileName);
         }
      }
   }




void ParseParameter (char *szParameter)
   {
   while ('\0' != *szParameter)
      {
      switch (*szParameter)
         {
         case '?':
            bPause = TRUE;
            if (szParameter[1] == '?')
               PrintShortHelp();
            else
               PrintHelp();
            exit (0);
            break;

         case 'h':
         case 'H': 
            bHidden = TRUE;
            break;

         case 'r':
         case 'R': 
            bRecurse    = TRUE;
            bSearchPath = FALSE;
            break;

         case 't':
            bTotal = TRUE;
            break;

         case 'T': 
            bTotal = TRUE;
            bVeryQuiet = TRUE;
            break;

         case 'e':
         case 'E': 
            bEveryHardDisk = TRUE;
            break;

         case 'd':
         case 'D':
            bDirectories = TRUE;
            break;

         case 'o':
         case 'O': 
            bOnlyDirectories = TRUE;
            break;

         case 'x':
         case 'X': 
            bExcludeTemps = TRUE;
            break;

         case 'q':
         case 'Q': 
            bQuiet = TRUE;
            break;

         case 'k':
         case 'K': 
            bKill = TRUE;
            break;

         case 'c':
         case 'C': 
            bCSV = TRUE;
            break;

         case 'p':
         case 'P': 
            bPause = TRUE;
            break;

         case 'm':
         case 'M': 
            strcpy (szCmd, szParameterValue (szParameter));
            bCmd = TRUE;
            return;

         case 'n':
         case 'N':
            bLineCount = TRUE;
            break;

         case 'l':
         case 'L': 
            ulMinimum = atol (szParameterValue (szParameter));
            return;
            break;

         case 's':
         case 'S': 
            ulMaximum = atol (szParameterValue (szParameter));
            return;
            break;

         case 'a':
         case 'A': 
            ParseDateTime (szParameterValue (szParameter), &After);
            return;
            break;

         case 'b':
         case 'B': 
            ParseDateTime (szParameterValue (szParameter), &Before);
            return;
            break;

         case 'w':
         case 'W': 
            strcpy (szWildCard, szParameterValue (szParameter));
            return;
            break;

         case 'z':
         case 'Z':
            bSearchPath    = TRUE;
            bRecurse       = FALSE;
            bEveryHardDisk = FALSE;
            break;

         default: 
            fprintf (stderr, "Unrecognized parameter %c ignored.\n",
                     *szParameter);
            break;
         }
      szParameter++;
      }
   }




char *szParameterValue (char *szParameter)
   {
   char *pEqual;

   pEqual = szParameter;
   while ('\0' != *pEqual && '=' != *pEqual)
      pEqual++;

   if ('=' == *pEqual)
      pEqual++;

   return pEqual;
   }




BOOL bTempDir (char *szDirName)
   {
   return (0 == strcmp (szDirName, "TMP")  ||
           0 == strcmp (szDirName, "TEMP") ||
           0 == strcmp (szDirName, "TEMPORARY"));
   }





void ParseDateTime (char *szDateTime, DateAndTime *datResult)
   {
   char *pNext;
   UINT iMonth;
   UINT iDay;
   UINT iYear;
   UINT iHours;
   UINT iMinutes;
   UINT iSeconds;
   DATETIME dtDateTime;

   pNext = szDateTime;


   if (isdigit (*pNext))
      {
      iMonth = 0;
      while (isdigit (*pNext))
         {
         iMonth *= 10;
         iMonth += *pNext - '0';
         pNext++;
         }
      if ('\0' != *pNext && NULL != strchr("/-,.", *pNext))
         pNext++;
   
      iDay = 0;
      while (isdigit (*pNext))
         {
         iDay *= 10;
         iDay += *pNext - '0';
         pNext++;
         }
      if ('\0' != *pNext && NULL != strchr("/-,.", *pNext))
         {
         pNext++;
         iYear = 0;
         while (isdigit (*pNext))
            {
            iYear *= 10;
            iYear += *pNext - '0';
            pNext++;
            }
         if (iYear < 100)
            {
            if (iYear < 80)
               iYear += 2000;
            else
               iYear += 1900;
            }
         if (iYear < 1980)
            iYear = 1980;
         iYear -= 1980;
         }
      else
         {
         DosGetDateTime (&dtDateTime);
         iYear  = dtDateTime.year-1980;
         }
      }
   else
      {
      switch (*pNext)
         {
         case 't':
         case 'T': DosGetDateTime (&dtDateTime);
                   iYear  = dtDateTime.year-1980;
                   iMonth = dtDateTime.month;
                   iDay   = dtDateTime.day;
                   break;
         case 'y':
         case 'Y': DosGetDateTime (&dtDateTime);
                   iYear  = dtDateTime.year-1980;
                   iMonth = dtDateTime.month;
                   iDay   = dtDateTime.day;
                   if (iDay > 1)
                      iDay--;
                   else if (iMonth > 1)
                      {
                      iMonth--;
                      switch (iMonth)
                        {
                        case  1:
                        case  3:
                        case  5:
                        case  7:
                        case  8:
                        case 10:
                        case 12: iDay = 31;
                                 break;

                        case  2: iDay = 28;
                                 if (iYear % 4 == 0)
                                    iDay = 29;
                                 break;

                        default: iDay = 30;
                        }
                      }
                   else
                     {
                     iYear--;
                     iMonth = 12;
                     iDay = 31;
                     }
                   break;
         default:
                   break;
         }
      while (isalpha(*pNext))
         pNext++;
      }

   if (iDay < 1 || iMonth < 1 || iMonth > 12 || iDay > 31)
      {
      fprintf (stderr, "Invalid date.\n");
      exit (ERROR_INVALID_DATE);
      }

   iHours = 0;
   iMinutes = 0;
   iSeconds = 0;

   if ('\0' != *pNext && ':' == *pNext)
      {
      pNext++;
      while (isdigit (*pNext))
         {
         iHours *= 10;
         iHours += *pNext - '0';
         pNext++;
         }
      if ('\0' != *pNext && ':' == *pNext)
         {
         pNext++;
         while (isdigit (*pNext))
            {
            iMinutes *= 10;
            iMinutes += *pNext - '0';
            pNext++;
            }
         if ('\0' != *pNext && ':' == *pNext)
            {
            pNext++;
            while (isdigit (*pNext))
               {
               iSeconds *= 10;
               iSeconds += *pNext - '0';
               pNext++;
               }
            }
         }
      }

   datResult->date.day   = iDay;
   datResult->date.month = iMonth;
   datResult->date.year  = iYear;

   datResult->time.hours   = iHours;
   datResult->time.minutes = iMinutes;
   datResult->time.twosecs = iSeconds / 2;
   }




void PrintHelp(void)
   {
   PRINTF ("%s\n", "This command lists all the files in specified directories last modified");
   PRINTF ("%s\n", "on specified dates.  The output consists of one line per file, with the");
   PRINTF ("%s\n", "file size, last date modified and fully qualified file name");
   PRINTF ("%s\n", "(including drive letter).");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "Usage:");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "direct [/cdehopqrtx? /w=<wildcards> /l=<number> /s=<number>");
   PRINTF ("%s\n", "        /b=<date-time> /a=<date-time]  {<dir-name>}");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "All parameters are optional and may be in any order.  Case of letters");
   PRINTF ("%s\n", "is not significant.  Single-letter commands (/c, etc.) may be combined");
   PRINTF ("%s\n", "or not, so \"direct /e /r\" and \"direct /er\" are equivalent.  Hyphens");
   PRINTF ("%s\n", "may be substituted for slashes.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "The single-letter parameters are:");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /c -- output in Comma-separated value format.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /d -- indicates that subDirectories should be listed as files.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /e -- search Every hard disk drive, from the root.  In this case,");
   PRINTF ("%s\n", "            specified directories are ignored.  Direct will think");
   PRINTF ("%s\n", "            that network drives are hard disk drives if their drive");
   PRINTF ("%s\n", "            letters are contiguous with the hard disk drive letters.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /h -- lists Hidden and system files, in addition to normal files.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /k -- Kill (delete) matching files.  Prompts for verification.\n");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /n -- counts the liNes in each file.  Combine with /t to get a total.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /o -- Only directories are listed.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /p -- output Pauses after each screenful, and Q quits the listing.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /q -- output is Quiet, with only file names and no sizes or dates.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /r -- Recursively searches subdirectories.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /t -- counts files, Totals sizes and allocations.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /T -- counts files, Totals sizes and allocations, very quitely.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /x -- eXcludes searching temporary subdirectories, those named");
   PRINTF ("%s\n", "            TMP, TEMP or TEMPORARY.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /z -- Search for files along PATH. Disables /r and /e");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /? -- prints Help.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /\?\?-- prints condensed Help.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "The parameters that take values are:");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /w=<wildcards> Specifies that only files matching the wildcards");
   PRINTF ("%s\n", "                  indicated are to be listed.  Multiple wildcards may");
   PRINTF ("%s\n", "                  be included by separated with semicolons, as in");
   PRINTF ("%s\n", "                     /w=*.h;*.c");
   PRINTF ("%s\n", "                  Wildcards are matched as in UNIX, so that the");
   PRINTF ("%s\n", "                  wildcard *x* will match only filenames with an x");
   PRINTF ("%s\n", "                  in the name or extension.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /l=<number>    Only files whose size is larger than or equal to");
   PRINTF ("%s\n", "                  the number are listed.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /s=<number>    Only files whose size is smaller than or equal to");
   PRINTF ("%s\n", "                  the number are listed.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /b=<date-time> Only files last modified on or before the date-time");
   PRINTF ("%s\n", "                  are listed.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /a=<date-time> Only files last modified on or after the date-time");
   PRINTF ("%s\n", "                  are listed.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   <date-time>    Must be specified in the following format: ");
   PRINTF ("%s\n", "                     Format                 Example");
   PRINTF ("%s\n", "                     ---------------------- ----------------------");
   PRINTF ("%s\n", "                     mm-dd-yyyy:hh:mm:ss.s  11-15-1990:15:12:31.5");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "                  The hyphens can be replaced by slashes or commas,");
   PRINTF ("%s\n", "                  and embedded spaces are NOT allowed.  The time can");
   PRINTF ("%s\n", "                  omitted, in which case it is assumed to be 00:00:00.0.");
   PRINTF ("%s\n", "                  If the time is omitted, there should be no colons");
   PRINTF ("%s\n", "                  specified.  The year can be omitted, in which case");
   PRINTF ("%s\n", "                  the current year will be assumed.  The date can be");
   PRINTF ("%s\n", "                  given as TODAY or YESTERDAY.  Case of letters is");
   PRINTF ("%s\n", "                  NOT significant.");
   }


void PrintShortHelp(void)
   {
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "Short Help for DIRECT Options:");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /c -- output in Comma-separated value format.");
   PRINTF ("%s\n", "   /d -- indicates that subDirectories should be listed as files.");
   PRINTF ("%s\n", "   /e -- search Every hard disk drive, from the root.");
   PRINTF ("%s\n", "   /h -- lists Hidden and system files, in addition to normal files.");
   PRINTF ("%s\n", "   /k -- Kill (delete) matching files.  Prompts for verification.");
   PRINTF ("%s\n", "   /n -- counts the liNes in each file.  Combine with /t to get a total.");
   PRINTF ("%s\n", "   /o -- Only directories are listed.");
   PRINTF ("%s\n", "   /p -- output Pauses after each screenful, and Q quits the listing.");
   PRINTF ("%s\n", "   /q -- output is Quiet, with only file names and no sizes or dates.");
   PRINTF ("%s\n", "   /r -- Recursively searches subdirectories.");
   PRINTF ("%s\n", "   /t -- counts files, Totals sizes and allocations.");
   PRINTF ("%s\n", "   /T -- counts files, Totals sizes and allocations, very quitely.");
   PRINTF ("%s\n", "   /x -- eXcludes dirs named  TMP, TEMP or TEMPORARY.");
   PRINTF ("%s\n", "   /z -- Search for files along PATH. Disables /r and /e.");
   PRINTF ("%s\n", "   /? -- prints Help.");
   PRINTF ("%s\n", "   /\?\?-- prints Condensed Help.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   /w=<wildcards> Specifies that only files matching the wildcards.");
   PRINTF ("%s\n", "   /l=<number>    Files whose size is larger than number.");
   PRINTF ("%s\n", "   /s=<number>    Files whose size is smaller than number.");
   PRINTF ("%s\n", "   /b=<date-time> files modified on or before date-time.");
   PRINTF ("%s\n", "   /a=<date-time> files modified on or after date-time.");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "   <date-time>    Must be specified in the following format: ");
   PRINTF ("%s\n", "                     Format                 Example");
   PRINTF ("%s\n", "                     ---------------------- ----------------------");
   PRINTF ("%s\n", "                     mm-dd-yyyy:hh:mm:ss.s  11-15-1990:15:12:31.5");
   PRINTF ("%s\n", "");
   PRINTF ("%s\n", "                  The hyphens can be replaced by slashes or commas,");
   PRINTF ("%s\n", "                  and embedded spaces are NOT allowed.  The time can");
   PRINTF ("%s\n", "                  omitted, in which case it is assumed to be 00:00:00.0.");
   PRINTF ("%s\n", "                  If the time is omitted, there should be no colons");
   PRINTF ("%s\n", "                  specified.  The year can be omitted, in which case");
   PRINTF ("%s\n", "                  the current year will be assumed.  The date can be");
   PRINTF ("%s\n", "                  given as TODAY or YESTERDAY.  Case of letters is");
   PRINTF ("%s\n", "                  NOT significant.");
   }








BOOL bMatchWildCards (char *szString, PRX *prxWildCards)
   {
   while (NULL != *prxWildCards)
      {
      if (bMatchWildCard (szString, *prxWildCards))
         return TRUE;
      prxWildCards++;
      }
   return FALSE;
   }




BOOL bMatchWildCard (char *szString, PRX prxWildCard)
   {
   return _regMatch (szString, prxWildCard);

//   switch (*szWildCard)
//      {
//      case '\0':
//         return '\0' == *szString;
//         break;
//
//      case '?':
//         return '\0' != *szString &&
//                bMatchWildCard (szString+1, szWildCard+1);
//         break;
//
//      case '*':
//         if ('\0' == szWildCard[1])
//            return TRUE;
//         while ('\0' != *szString)
//            {
//            if (bMatchWildCard (szString, szWildCard+1))
//               return TRUE;
//            szString++;
//            }
//         return bMatchWildCard (szString, szWildCard+1);
//         break;
//
//      default:
//         if ('\0' == *szString)
//            return FALSE;
//         return (toupper(*szString) == toupper(*szWildCard)) &&
//                bMatchWildCard (szString+1, szWildCard+1);
//         break;
//      }
//
//   return TRUE;
   }



char *pMakeCSVField (char *pszString, char *pResult)
   {
   BOOL bNeedsQuotes;
   char *pChar;

   bNeedsQuotes = FALSE;

   strcpy (pResult, pszString);

   for (pChar = pResult; '\0' != *pChar; pChar++)
      {
      if ('"' == *pChar)
         {
         InsertChar ('"', pChar);
         pChar++;
         bNeedsQuotes = TRUE;
         }
      if (',' == *pChar)
         bNeedsQuotes = TRUE;
      }

   if (bNeedsQuotes)
      {
      *pChar = '"';
      *(pChar+1) = '\0';
      InsertChar ('"', pResult);
      }

   return pResult;
   }



static void InsertChar (char c, char *pszString)
   {
   char *pTemp;

   pTemp = pszString;

   while ('\0' != *pTemp)
      pTemp++;

   *(pTemp+1) = '\0';

   while (pTemp >= pszString)
      {
      *(pTemp+1) = *pTemp;
      pTemp--;
      }

   *pszString = c;
   }



void Pause (void)
   {
   char c;
   KBDKEYINFO kbci;

   printf ("--More--");

   KbdCharIn(&kbci, IO_WAIT, 0);
   c = kbci.chChar;

   printf ("\b\b\b\b\b\b\b\b");

   if ('q' == c || 'Q' == c)
      exit (3);

   if (' ' == c)
      usCurrRow = 1;
   else
      usCurrRow--;
   }



static ULONG CountLines (char *pszFile)
   {
   FILE *pf;
   int  c;
   ULONG ulLines;

   pf = _fsopen (pszFile, "r", SH_DENYNO);
   if (pf == NULL)
      return 0;

   ulLines = 0;
   while (EOF != (c = fgetc (pf)))
      if (c == '\n')
         ulLines++;

   fclose (pf);
   return ulLines;
   }



static void MakeNormal (PSZ pszFile)
   {
   HFILE       hf;
   FILESTATUS  fsts;
   USHORT      usError, usAction;

   usError = DosOpen (pszFile, &hf, &usAction, 0, FILE_NORMAL, 
                      FILE_OPEN, 
                      OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE, 0);
   if (usError)
      return;

   usError = DosQFileInfo (hf, FIL_STANDARD, &fsts, sizeof fsts);
   if (usError == 0)
      {
      fsts.attrFile = FILE_NORMAL;
      usError = DosSetFileInfo (hf, FIL_STANDARD, (PBYTE) &fsts, sizeof fsts);
      }

   DosClose (hf);
   }



static void KillFile (FILEFINDBUF *pFile)
   {
   static BOOL bAsk = TRUE;
   BOOL bOK;
   BOOL bDone;
   int c;

   bOK = FALSE;
   bDone = FALSE;
   if (bAsk)
      {
      if (bQuiet)
         {
         printf ("Delete %s? ", pFile->achName);
         }
      else
         {
         printf ("Delete? [ynAq?]");
         }

      while (!bDone)
         {
         c = getch ();
         switch (c)
            {
            case 'y':
            case 'Y':
               bOK = TRUE;
               bDone = TRUE;
               break;

            case 'n':
            case 'N':
               bDone = TRUE;
               break;

            case 'q':
            case 'Q':
               exit (0);
               break;

            case 'A':
               bAsk = FALSE;
               bOK = TRUE;
               bDone = TRUE;
               break;
            }
         }
      printf ("%c\n", (char) c);
      }

   if (bOK || !bAsk)
      {
      if (pFile->attrFile & FILE_READONLY || 
          pFile->attrFile & FILE_SYSTEM || 
          pFile->attrFile & FILE_HIDDEN)
         {
         MakeNormal (pFile->achName);
         }
                    
      if (pFile->attrFile & FILE_DIRECTORY)
         DosRmDir (pFile->achName, 0);
      else
         DosDelete (pFile->achName, 0);
      }
   }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Since a MARC record can be at most 99999 chars:
  LEADERSIZE + (DIRENTRYSIZE * # of direntries) + (1 [min field size] * # of direntries) <= 99999
  max # of direntries is 7690
  max DIRSIZE = 92290
*/

#define DIRENTRYSIZE 12
#define LEADERSIZE 24
#define MAXDIRSIZE 92290
#define MAXFIELDSIZE 9999
#define MAXFILENAMESIZE 50
#define MAXRECORDSIZE 99999
#define TAGSIZE 3

const int RT = 29; /* MARC end-of-record marker */
const int FT = 30; /* MARC end-of-field marker */
const int SF = 31; /* MARC subfield marker */

char* SetFilename(char *Filename, char *arg);
void ShowUsage(void);

int main(int argc, char *argv[])
{
	int c, i;
	long BaseAddress, DirSize, FieldSize, FieldStart, RecordNumber, RecordPos;
	char InFilename[MAXFILENAMESIZE + 1] = "\0"; /* plus \0 */
	char Directory[MAXDIRSIZE + 1] = "\0";
	char DirEntry[DIRENTRYSIZE + 1] = "\0";
	char Field[MAXFIELDSIZE + 1] = "\0";
	char Record[MAXRECORDSIZE + 1] = "\0";
	char Tag[TAGSIZE + 1] = "\0";
	char TEMP[10 + 1] = "\0";
	FILE *infile;

	if (argc != 2)
	{
		ShowUsage();
		exit (1);
	}

	SetFilename(InFilename, argv[1]);
	infile = fopen(InFilename, "rb");

	if ( ! infile )
	{
		printf("\nCannot open %s - exiting\n", InFilename);
		exit (1);
	}

	RecordPos = 0;
	RecordNumber = 0;
	while ( (c = getc(infile)) != EOF )
	{
		if (c == SF) c = '$'; /* convert subfield marker to more readable character */
		Record[RecordPos] = c;
		if (c == RT)
		{
			Record[++RecordPos] = 0; /* end of string */
			RecordNumber++;
			printf("--- Record number %d ---\n", RecordNumber);
			strncpy(Field, Record, LEADERSIZE);
			Field[LEADERSIZE] = 0;
			printf("LDR:%s\n", Field);

			/* BaseAddress = atol(strncpy(Field, Record + 12, 5)); */
			strncpy(Field, Record + 12, 5);
			Field[5] = 0;
			BaseAddress = atol(Field);
			DirSize = BaseAddress - LEADERSIZE - 1;
			strncpy(Directory, Record + LEADERSIZE, DirSize);
			Directory[DirSize] = 0;

			for (i = 0; i < (DirSize / DIRENTRYSIZE); i++)
			{
				strncpy(DirEntry, Directory + (i * DIRENTRYSIZE), DIRENTRYSIZE);
				DirEntry[DIRENTRYSIZE] = 0;
				strncpy(Tag, DirEntry + 0, 3);
				Tag[TAGSIZE] = 0;
				strncpy(TEMP, DirEntry + 3, 4);
				TEMP[4] = 0;
				FieldSize = atol(TEMP);
				strncpy(TEMP, DirEntry + 7, 5);
				TEMP[5] = 0;
				FieldStart = atol(TEMP);
				/* don't want end-of-field marker */
				strncpy(Field, Record + BaseAddress + FieldStart, FieldSize - 1);
				Field[FieldSize - 1] = 0;
				printf("%s:%s\n", Tag, Field);
			}
			printf("\n");
			RecordPos = 0; /* prepare for next record */
		}
		else
		{
			RecordPos++;
		}

		if (RecordPos >= MAXRECORDSIZE)
		{
			printf("\nERROR: record %d is too large - exiting\n", ++RecordNumber);
			fclose(infile);
			exit (1);
		}
	}

	fclose(infile);
	return 0;
}

/*=============================================================================
SetFilename: Copies command-line arg to filename variable, checking max length
=============================================================================*/
char* SetFilename(char *Filename, char *arg)
{
	int len = strlen(arg);
	if (len > MAXFILENAMESIZE)
	{
		printf("\nFilename too big (%s) - exiting\n", arg);
		exit (1);
	}
	else
	{
		strcpy(Filename, arg);
	}

	return Filename;
}

/*=============================================================================
ShowUsage: Show usage message
=============================================================================*/
void ShowUsage(void)
{
	int i;
	char const *Usage_Messages[] =
	{
		"Marcview: converts file of MARC records into text format",
		"",
		"USAGE: marcview marcinputfile"
	};

	for (i = 0; i < (sizeof(Usage_Messages) / sizeof*(Usage_Messages)); i++)
	{
		printf("%s\n", Usage_Messages[i]);
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXFILENAMESIZE 50
#define EXTENSIONSIZE 4

const int RT = 29; /* MARC end-of-record marker */

long CountRecords(FILE *file);
char* SetFilename(char *Filename, char *arg);
void ShowUsage(void);

int main(int argc, char *argv[])
{
	int c, i, len;
	int NumToRead, NumOfChunks, RecordCount;
	int NumWritten = 0;
	char Mode;
	char InFilename[MAXFILENAMESIZE + 1] = "\0"; /* plus \0 */
	char BaseFilename[MAXFILENAMESIZE + 1] = "\0";
	char Extension[EXTENSIONSIZE + 1] = "\0";
	char OutFilename[MAXFILENAMESIZE + EXTENSIONSIZE + 1] = "\0";
	FILE *infile;
	FILE *outfile;

	if (argc < 2)
	{
		ShowUsage();
		exit (1);
	}

	if (argv[1][0] != '-')
	{
		ShowUsage();
		exit (1);
	}

	Mode = argv[1][1];

	switch (Mode)
	{
		case 'c' :	/* display count of records only */
			if (argc != 3)
			{
				ShowUsage();
				exit (1);
			}
			SetFilename(InFilename, argv[2]);
			if ( !(infile = fopen(InFilename, "rb")) )
			{
				printf("\nCannot open %s - exiting\n", InFilename);
				exit (1);
			}
			RecordCount = CountRecords(infile);
			printf("\n%s contains %d records\n", InFilename, RecordCount);
			fclose(infile);
			break;
		case 'f' : /* write first n records to output file */
			if (argc != 5)
			{
				ShowUsage();
				exit (1);
			}
			NumToRead = atoi(argv[2]);
			SetFilename(InFilename, argv[3]);
			SetFilename(OutFilename, argv[4]);
			if ( !(infile = fopen(InFilename, "rb")) )
			{
				printf("\nCannot open %s - exiting\n", InFilename);
				exit (1);
			}
			outfile = fopen(OutFilename, "wb");
			while ( (c = getc(infile)) != EOF )
			{
				putc(c, outfile);
				if (c == RT)
				{
					NumWritten++;
					if (NumWritten == NumToRead)
						break;
				}
			}

			fclose(outfile);
			fclose(infile);
			break;
		case 's' : /* split file into chunks of n records */
			if (argc != 5)
			{
				ShowUsage();
				exit (1);
			}
			NumToRead = atoi(argv[2]);
			SetFilename(InFilename, argv[3]);
			SetFilename(BaseFilename, argv[4]);
			if ( !(infile = fopen(InFilename, "rb")) )
			{
				printf("\nCannot open %s - exiting\n", InFilename);
				exit (1);
			}
			RecordCount = CountRecords(infile);
			NumOfChunks = (int)((RecordCount - 1) / NumToRead) + 1;
			if ( NumOfChunks > 999 )
			{
				printf("\nCannot split files into more than 999 pieces.");
				printf("\n%s contains %d records, which would be %d files of %d records.", InFilename, RecordCount, NumOfChunks, NumToRead);
				printf("\nPlease use a larger value (at least %d)\n", (int)(RecordCount / 999) + 1);
				exit (1);
			}

			for (i = 1; i <= NumOfChunks; i++)
			{
				strcpy(OutFilename, BaseFilename);
				sprintf(Extension, "%#03d", i);
				strcat(OutFilename, ".");
				strcat(OutFilename, Extension);
				printf("%s\n", OutFilename);
				outfile = fopen(OutFilename, "wb");
				NumWritten = 0;
				while ( ((c = getc(infile)) != EOF) && (NumWritten < NumToRead) )
				{
					putc(c, outfile);
					if (c == RT)
					{
						NumWritten++;
						if (NumWritten == NumToRead)
							break;
					}
				}
				fclose(outfile);
			}
			fclose(infile);
			break;
		default :
			ShowUsage();
			exit (1);
	}
	return 0;
}

/*=============================================================================
CountRecords: counts the number of MARC records in a file
=============================================================================*/
long CountRecords(FILE *file)
{
	int c;
	long count = 0;
	fseek(file, 0L, SEEK_SET);
	while ( (c = getc(file)) != EOF )
	{
		if (c == RT)
			count++;
	}
	fseek(file, 0L, SEEK_SET); /* return file pointer to beginning */
	return count;
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
		"Marcsplit: breaks file of MARC records into smaller files",
		"",
		"USAGE: marcsplit -[c|f|s] marcinputfile [marcoutputfile]",
		"  -c          Displays count of records",
		"  -f number   Copies first [number] records from inputfile to outputfile",
		"  -s number   Copies inputfile into chunks of [number] records;",
		"                writes up to 999 outputfiles with format of:",
		"                outputfile.001, outputfile.002, etc."
	};

	for (i = 0; i < (sizeof(Usage_Messages) / sizeof*(Usage_Messages)); i++)
	{
		printf("%s\n", Usage_Messages[i]);
	}
}

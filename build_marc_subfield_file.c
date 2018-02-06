#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
  Since a MARC record can be at most 99999 chars:
  LEADER_SIZE + (DIR_ENTRY_SIZE * # of direntries) + (1 [min field size] * # of direntries) <= 99999
  max # of direntries is 7690
  MAX_DIR_SIZE = 92290
*/

#define DIR_ENTRY_SIZE			12
#define INDICATOR_SIZE			 2
#define LEADER_SIZE				24
#define MAX_DIR_SIZE		 92290
#define MAX_FIELD_SIZE		  9999
#define MAX_FILENAME_SIZE	   255
#define MAX_RECORD_SIZE		 99999
#define MAX_SUBFIELD_SIZE	  2000	/* for Oracle: NVARCHAR2() with AL16UTF16 charset; not worth using CLOB for this purpose*/
#define SUBFIELD_CODE_SIZE		 1
#define TAG_SIZE				 4	/* for SQL DB: 3 for MARC tag, 1 for subfield code */

const int RT = 29; /* MARC end-of-record marker */
const int FT = 30; /* MARC end-of-field marker */
const int SF = 31; /* MARC subfield marker */

void PrintRow(FILE* OutputFile, long RecordID, int FieldSeq, int SubfieldSeq, char* Indicators, char* Tag, char* Field);
char* SetFilename(char* Filename, char* arg);
void ShowUsage(void);

int main(int argc, char *argv[])
{
	unsigned char ch;
	int i = 0;
	int DirEntryNum = 0;
	int ReadChar = 0;
	int FieldPos = 0;
	int SubfieldPos = 0;
	int FieldSeq = 0;
	int SubfieldSeq = 0;
	long BaseAddress = 0;
	long DirSize = 0;
	long FieldSize = 0;
	long FieldStart = 0;
	long RecordCount = 0;
	long RecordPos = 0;
	long RecordID = 0;
	char InputFilename[MAX_FILENAME_SIZE + 1] = "\0";
	char OutputSubfieldFilename[MAX_FILENAME_SIZE + 1] = "\0";
	char Directory[MAX_DIR_SIZE + 1] = "\0";
	char DirEntry[DIR_ENTRY_SIZE + 1] = "\0";
	char Field[MAX_FIELD_SIZE + 1] = "\0";
	char Indicators[INDICATOR_SIZE + 1] = "\0";
	char Leader[LEADER_SIZE + 1] = "\0";
	char Record[MAX_RECORD_SIZE + 1] = "\0";
	char RecordType[4] = "\0";
	char Subfield[MAX_SUBFIELD_SIZE + 1] = "\0";
	char SubfieldCode[SUBFIELD_CODE_SIZE + 1] = "\0";
	char Tag[TAG_SIZE + 1] = "\0";
	char tmpbuf[MAX_RECORD_SIZE + 1] = "\0";
	FILE* InputFile;
	FILE* OutputSubfieldFile;

	if (argc != 3)
	{
		ShowUsage();
		exit (1);
	}

	SetFilename(InputFilename, argv[1]);
	SetFilename(OutputSubfieldFilename, argv[2]);

	/* Use BINARY (b) mode - if not specified, defaults to TEXT */
	InputFile = fopen(InputFilename, "rb");
	OutputSubfieldFile = fopen(OutputSubfieldFilename, "wb");

	if ( ! InputFile )
	{
		printf("\nCannot open %s - exiting\n", InputFilename);
		exit (1);
	}

	RecordPos = 0;
	RecordCount = 0;
	while ( (ReadChar = getc(InputFile)) != EOF )
	{
		Record[RecordPos] = ReadChar;
		if (ReadChar == RT)
		{
			Record[++RecordPos] = 0; /* end of string */
			RecordCount++;
			FieldSeq = 1;
			SubfieldSeq = 1;

			/* Get the leader (bytes 0-23 of record) */
			strncpy(Leader, Record, LEADER_SIZE);
			Leader[LEADER_SIZE] = 0;

			/* Get the base address of data: bytes 12-16 of leader */
			strncpy(tmpbuf, Leader + 12, 5);
			tmpbuf[5] = 0;
			BaseAddress = atol(tmpbuf);

			/* Get the directory: bytes 25 - (BaseAddress - 1) of record */
			DirSize = BaseAddress - LEADER_SIZE - 1;
			strncpy(Directory, Record + LEADER_SIZE, DirSize);
			Directory[DirSize] = 0;

			/* Iterate through the directory entries to get each tag & field */
			for (DirEntryNum = 1; DirEntryNum <= (DirSize / DIR_ENTRY_SIZE); DirEntryNum++)
			{
				/* Directory consists of 12-bytes entries, which need to be chopped
				   into 3/4/5-byte chunks (Tag: 0-2; FieldSize: 3-6; FieldStart: 7-11)
				*/
				strncpy(DirEntry, Directory + ((DirEntryNum - 1) * DIR_ENTRY_SIZE), DIR_ENTRY_SIZE);
				DirEntry[DIR_ENTRY_SIZE] = 0;

				/* For SQL DB, storing Tag & subfield code in same 4-char string
				   Some fields don't have subfields, so when initializing Tag, null-terminate after
				   3 chars
				*/
				strncpy(Tag, DirEntry + 0, 3);
				Tag[TAG_SIZE - 1] = 0;

				strncpy(tmpbuf, DirEntry + 3, 4);
				tmpbuf[4] = 0;
				FieldSize = atol(tmpbuf);

				strncpy(tmpbuf, DirEntry + 7, 5);
				tmpbuf[5] = 0;
				FieldStart = atol(tmpbuf);

				/* Don't want end-of-field marker so reduce FieldSize */
				FieldSize--;

				/* Use temp buffer at first, as we'll trim indicators from start of variable-length fields */
				strncpy(tmpbuf, Record + BaseAddress + FieldStart, FieldSize);
				tmpbuf[FieldSize] = 0;

				/* Only fields 010 & higher have indicators (first two chars of field) */
				if (atoi(Tag) <= 9) {
					Indicators[0] = 0;
					/* strcpy OK since tmpbuf is already null-terminated & can't be bigger than Field */
					strcpy(Field, tmpbuf);
					/*	Get record number from 001
						Assumes 001 exists, and that its contents are numerical - OK for this purpose
					*/
					if (atoi(Tag) == 1) {
						RecordID = atol(Field);
					}
				}
				else {
					strncpy(Indicators, tmpbuf, INDICATOR_SIZE);
					Indicators[INDICATOR_SIZE] = 0;
					/* Trim indicators from beginning of field */
					/* strcpy OK since tmpbuf is already null-terminated & can't be bigger than Field */
					strcpy(Field, tmpbuf + INDICATOR_SIZE);
				}

				FieldSeq++;

				/* Build subfield(s) from field, replacing certain characters as we go */
				SubfieldPos = 0;
				SubfieldSeq = 1;

				for (FieldPos = 0; FieldPos <= strlen(Field); FieldPos++) {
					ch = Field[FieldPos];
					if (ch == SF) {
						/* Output the previous subfield (if it exists) */
						if (FieldPos != 0) {
							Subfield[SubfieldPos] = 0;
							PrintRow(OutputSubfieldFile, RecordID, FieldSeq, SubfieldSeq, Indicators, Tag, Subfield);
							SubfieldPos = 0;
							SubfieldSeq++;
						}
						/* Grab the next char (subfield code) and append to Tag, then add null-terminator */
						FieldPos++;
						Tag[TAG_SIZE - 1] = Field[FieldPos];
						Tag[TAG_SIZE] = 0;
					}
					else {
						if (SubfieldPos < MAX_SUBFIELD_SIZE) {
							Subfield[SubfieldPos] = ch;
							SubfieldPos++;
						}
						else {
							fprintf(stderr, "Subfield truncated: record %d, tag %s\n", RecordID, Tag);
							break; /* out of the FieldPos loop */
						}
					} /* end of else (ch != SF) */
				} /* end of for FieldPos */

				Subfield[SubfieldPos] = 0;
				PrintRow(OutputSubfieldFile, RecordID, FieldSeq, SubfieldSeq, Indicators, Tag, Subfield);

			} /* end of for DirEntryNum */

			/*	Print the leader, with special values
				Couldn't print it earlier as RecordID isn't known until fields are parsed
				Printing LDR last doesn't matter for this purpose, unless index-aware bulk loading
				requiring ordered FieldSeq will be done.
			*/
			FieldSeq = 1;
			SubfieldSeq = 1;
			strncpy(Tag, "LDR", TAG_SIZE);
			Tag[TAG_SIZE] = 0;
			Indicators[0] = 0;
			PrintRow(OutputSubfieldFile, RecordID, FieldSeq, SubfieldSeq, Indicators, Tag, Leader);

			/* Prepare for next record */
			RecordPos = 0;
		} /* end of if RT */
		else
		{
			RecordPos++;
		}

		if (RecordPos >= MAX_RECORD_SIZE)
		{
			printf("\nERROR: record %d is too large - exiting\n", ++RecordCount);
			fclose(InputFile);
			exit (1);
		}
	}

	fclose(InputFile);
	printf("%s: processed %d records\n", InputFilename, RecordCount);
	return 0;
}

/*=============================================================================
PrintRow: Sends subfield data to output stream, with an extra \0 (used as delimiter for loading into SQL)
=============================================================================*/
void PrintRow(FILE* OutputFile, long RecordID, int FieldSeq, int SubfieldSeq, char* Indicators, char* Tag, char* Field)
{
	//fprintf(OutputFile, "%ld\t%d\t%d\t%s\t%s\t%s%c", RecordID, FieldSeq, SubfieldSeq, Indicators, Tag, Field, 0x00);
	fprintf(OutputFile, "%ld\t%d\t%d\t%s\t%s\t%s\n", RecordID, FieldSeq, SubfieldSeq, Indicators, Tag, Field);
}

/*=============================================================================
SetFilename: Copies command-line arg to filename variable, checking max length
=============================================================================*/
char* SetFilename(char* Filename, char* arg)
{
	int len = strlen(arg);
	if (len > MAX_FILENAME_SIZE)
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
		"build_marc_subfield_file: parses MARC records into subfields for load into SQL DB",
		"",
		"USAGE: build_marc_subfield_file input_marc_file output_subfield_file "
	};

	for (i = 0; i < (sizeof(Usage_Messages) / sizeof*(Usage_Messages)); i++)
	{
		printf("%s\n", Usage_Messages[i]);
	}
}

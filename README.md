# marc-c-utilities
Basic utility programs for MARC records, written in C.
All programs should be easily compilable with gcc.  Example:
`gcc marcview.c -o marcview`

### build_marc_subfield_file
Creates a tab-delimited file, one line for each subfield, for every MARC record in a file.
```
USAGE: build_marc_subfield_file input_marc_file output_subfield_file
```

Format of output: 6 columns, tab-delimited.  Columns:
```
# Record id (from 001 field); 0 if no 001
# Field sequence: integer representing position of that field in the current record
# Subfield sequence: integer representing position of that subfield in the current field
# Indicators: string containing the indicators (if field is 010-999) or empty string for LDR and 000-009 fields
# Tag and subfield: string containing the 3-character tag (or LDR for the leader) and a 4th character for the subfield, for 010-999 fields
# Content: string containing the content of the given subfield (or of the field, for LDR and 000-009 fields)
```
For example, the MARC field
`856 40 $z Open Access via eScholarship $u http://escholarship.org/uc/item/99n9k56z`
has output like this, where the record id (001) is 123, and the field is the 20th (counting the leader) in the record:
```
123       20      1       40      856z    Open Access via eScholarship
123       20      2       40      856u    http://escholarship.org/uc/item/99n9k56z
```

### marcsplit
Breaks a file of MARC records into smaller files.
```
USAGE: marcsplit -[c|f|s] marcinputfile [marcoutputfile]
  -c          Displays count of records
  -f number   Copies first [number] records from inputfile to outputfile
  -s number   Copies inputfile into chunks of [number] records;
                writes up to 999 outputfiles with format of:
                outputfile.001, outputfile.002, etc.
```

### marcview
Converts binary MARC to readable text.  Output goes to STDOUT.
```
USAGE: marcview marcinputfile
```

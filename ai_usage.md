#AI Usage Documentation — Phase 1

Tool Used
Claude (Anthropic)

Prompt 1 — parse_condition

Prompt given to AI:
 I have a C struct called REPORT with these fields: int id, char inspector_name[30], GPS coordinates (float latitude, float longitude), char issue_category[30], int severity_level, time_t timestamp, char descriptio [150]. A filter condition is a string of the form "field:operator:value", for example "severity:>=:2" or "category:==:road". Generate a C function: int parse_condition(const char *input, char *field, char *op, char *value); that splits the input string on ':' and fills in field, op, and value. Return 1 on success, 0 on failure.

What was generated:
The AI produced a function that used two calls to strchr() to find the first and second : delimiters, then used strncpy() to copy each segment. The function returned 0 if either delimiter was missing or if any segment was empty.


Prompt 2 — match_condition

Prompt given to AI:
 Using the same REPORT struct, please generate a C function: int match_condition(REPORT *r, const char *field, const char *op, const char *value); that returns 1 if the record satisfies the condition and 0 otherwise. Supported fields: severity (int), category (string), inspector (string), timestamp (time_t). Supported operators: ==, !=, <, <=, >, >=.

What was generated:
The AI produced a function using a series of if/else if blocks, one per field. For numeric fields (severity, timestamp) it converted value with atoi(). For string fields it used strcmp() and compared the result against 0.










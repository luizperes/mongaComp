extern int yylex(void);

typedef enum token 
{
	
	TK_STR = 501,
	TK_VAR

} Token;


typedef union seminfo_t
{
	int i;
	double d;
	const char *s;

} Seminfo_t;
extern Seminfo_t seminfo;
extern int yy_lines;
void lexError(const char* message, int ret);


#ifndef EALIB_H
#define EALIB_H

#ifndef numof
#  define numof(A) (sizeof(A)/sizeof((A)[0]))
#endif

/* Šg’£‘®«‚ğ“Ç‚İæ‚éˆ×‚Ìƒ|ƒCƒ“ƒ^Œ^ */
union MultiPtr {
  void *value;
  char *byte;
  unsigned short *word;
};

enum{ 
  EA_ASCIITYPE = 0xFFFD,
  EA_MULTITYPE = 0xFFDF,
};

char *get_ascii_ea(  const char *fname , const char *eatype );
char **get_multiascii_ea( const char *fname  , const char *eatype );
void free_pointors(char **p);
int set_ascii_ea(const char *fname,const char *eatype,const char *value );
int set_multiascii_ea(  const char *fname,const char *eatype
		      , const char **values);
int unset_ea( const char *fname , const char *eatype );

#endif

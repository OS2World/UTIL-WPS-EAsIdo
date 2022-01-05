/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>

#include "ealib.h"
#include "strbuffer.h"

#if 1
#  define debug(s)
#else
#  define debug(s) fputs(s,stderr)
#endif

struct UnknownTypeEA {
  const char *typeName;
  UnknownTypeEA( const char *s ) : typeName(s) { }
};

struct NotMultiTypeEA {
  const char *typeName;
  NotMultiTypeEA( const char *s ) : typeName(s) { }
};

char *gets2heap(FILE *fp)
{
  StrBuffer sb;
  int ch;
  if( feof(fp) )
    return NULL;
  while( (ch=getc(fp)) != EOF && ch != '\n' )
    sb << (char)ch;
  return sb.finish();
}

class EAManip {
  const char *name;
  const char *shortName;
public:
  struct FailToWrite{
    int rc;
    const char *fname,*type;
    FailToWrite(int _rc,const char *_name,const char *_type)
      : rc(_rc) , fname(_name) , type(_type){ }
  };

  virtual void set( const char *fname,const char *value) throw(FailToWrite)=0;
  virtual void read( const char *fname , FILE *fp) throw(FailToWrite)=0;
  virtual void print( const char *fname , FILE *fp )=0;
  virtual void list( const char *fname , FILE *fp )=0;

  EAManip( const char *_name , const char *_short )
    : name(_name) , shortName(_short) { }

  const char *getName()      const { return name; }
  const char *getShortName() const { return shortName; }

  bool operator == ( const char *name ){
    return (  name[0] == '.' 
	    ? stricmp(name,this->name)==0
	    : stricmp(name,this->name+1)==0
	    ) || stricmp(name,this->shortName)==0;
  }
  bool operator != ( const char *name ){
    return !(*this == name );
  }
};

class AsciiEAManip : public EAManip {
public:
  void set( const char *fname , const char *value ) throw(FailToWrite) {
    debug("ENTER MultiAsciiEAManip::set\n");
    int rc=set_ascii_ea(fname,getName(),value );
    if( rc != 0 )
      throw FailToWrite(rc,fname,getName());
  }
  void read( const char *fname , FILE *fp ) throw(FailToWrite) {
    char *line;
    if( !feof(fp)  &&  (line=gets2heap(fp)) != NULL ){
      int rc=set_ascii_ea(fname , getName() , line );
      free(line);
      if( rc != 0 )
	throw FailToWrite(rc,fname,getName());
    }
  }
  void print( const char *fname , FILE *fp ){
    char *value=get_ascii_ea(fname,getName());
    if( value != NULL){
      fputs(value,fp);
      putc('\n',fp);
      free(value);
    }
  }
  void list( const char *fname , FILE *fp ){
    char *ea=get_ascii_ea(fname,this->getName());
    if( ea != NULL ){
      fprintf(fp,"%s: %s\n",this->getName(),ea);
      free(ea);
    }
  }
  AsciiEAManip(const char *name,const char *sname)
    : EAManip(name,sname){ }
};

class MultiAsciiEAManip : public EAManip {
public:
  void set( const char *fname , const char *value ) throw(FailToWrite){
    const char *array[]={value,NULL};
    int rc=set_multiascii_ea( fname , getName() , array );
    if( rc != 0 )
      throw FailToWrite(rc,fname,getName());
  }
  void print( const char *fname , FILE *fp ){
    char **list=get_multiascii_ea(fname,getName() );
    if( list != NULL ){
      for(char **p=list ; *p != NULL ; ++p ){
	fputs(*p,fp);
	putc('\n',fp);
      }
      free_pointors(list);
    }
  }
  void read( const char *fname , FILE *fp)  throw(FailToWrite) {
    const char *_lines[256];
    char *lines[256];
    int nlines = 0;
    while(    nlines < numof(lines)  
	  &&  !feof(fp)
	  &&  (_lines[nlines]=lines[nlines] = gets2heap(fp)) != NULL ){
      ++nlines;
    }
    int rc=0;
    if( nlines > 0 ){
      rc=set_multiascii_ea( fname , getName() , _lines );
      if( rc != NULL )
	throw FailToWrite(rc,fname,getName());
      while( nlines > 0 )
	free( lines[ --nlines ] );
    }
  }
  void list( const char *fname , FILE *fp ){
    char **eas=get_multiascii_ea(fname,this->getName() );
    if( eas != NULL ){
      for(int i=0; eas[i] != NULL ; ++i ){
	fprintf(fp,"%s[%d]: %s\n",this->getName(),i,eas[i] );
      }
      free_pointors(eas);
    }
  }
  MultiAsciiEAManip(const char *name,const char *sname)
    : EAManip(name,sname) { }
};

EAManip *eaList[] = {
  new AsciiEAManip(      ".LONGNAME"   , "L" ) ,
  new AsciiEAManip(      ".SUBJECT"    , "S" ) ,
  new MultiAsciiEAManip( ".COMMENTS"   , "C" ) ,
  new MultiAsciiEAManip( ".HISTORY"    , "H" ) ,
  new MultiAsciiEAManip( ".KEYPHRASES" , "K" ) ,
  NULL
};

EAManip *which_eatype(const char *s) throw(UnknownTypeEA)
{
  for( EAManip **p=eaList ; *p != NULL ; ++p ){
    if( **p == s )
      return *p;
  }
  throw UnknownTypeEA(s);
}


void listing(const char *fname)
{
  for(EAManip **p=eaList ; *p != NULL ; ++p ){
    (*p)->list(fname,stdout);
  }
}

void set_ea( const char *fname , const char *eatype , const char *value )
     throw(UnknownTypeEA,EAManip::FailToWrite)
{
  debug("ENTER set_ea()\n");
  if( value == 0 || value[0] == '\0' ){
    debug("LEAVE set_ea() without set ea\n");
  }
   
  EAManip *eamanip=which_eatype(eatype);
  debug("LEAVE set_ea()\n");
  eamanip->set( fname , value );
}

void  remove_ea( const char *fname , const char *eatype ) 
     throw(UnknownTypeEA,EAManip::FailToWrite)
{
  EAManip *ea=which_eatype(eatype);
  
  int rc=unset_ea( fname , ea->getName() );
  if( rc )
    throw EAManip::FailToWrite(rc,fname,ea->getName() );
}

void append_ea( const char *fname , const char *eatype , const char *text )
     throw(UnknownTypeEA,EAManip::FailToWrite,NotMultiTypeEA)
{
  MultiAsciiEAManip *ea
    = dynamic_cast <MultiAsciiEAManip*> ( which_eatype(eatype) );

  if( ea==0 )
    throw NotMultiTypeEA( eatype );

  int n=0;  
  char **list=get_multiascii_ea( fname , ea->getName() );
  if( list != NULL ){
    
    for(char **p=list ; *p != NULL ; ++p )
      ++n;
    /* ここで n は行数になっているはずので、配列要素数は NULL を入れた(n+1)。
     * それが１つ増えるので (n+2) 個 realloc する必要がある。
     * なお、list[n] は 旧の NULL の位置。
     */
    list = (char**)realloc(list,sizeof(char*)*(n+2));
  }else{
    list = (char **)malloc(sizeof(char*)*2);
  }
  list[ n++ ] = strdup(text);
  list[ n   ] = NULL;

  set_multiascii_ea( fname , ea->getName() , (const char **)list );
  free_pointors( list );
}


void message_ignored( char *progname , int argc , char **argv )
{
  if( argc > 0 ){
    fprintf(stderr,"%s: ignored argument",progname);
    while( argc-- > 0 &&  *argv != NULL)
      fprintf(stderr," %s",*argv++);
  }
}

class NoSuchFileOrDirectory{
  char *name_;
public:
  const char *name() const { return name_; }
  
  NoSuchFileOrDirectory( const char *nm ) : name_( strdup(nm) ){}
  ~NoSuchFileOrDirectory() { free(name_); }
};

inline void check_file_existance( const char *path )
     throw( NoSuchFileOrDirectory )
{
  if( access( path , 0 ) != 0 )
    throw NoSuchFileOrDirectory( path );
}

enum{
  MAINRET_NO_ERROR ,
  MAINRET_NO_SUCH_FILE_OR_DIRECTORY ,
  MAINRET_NO_ARGUMENTS ,
  MAINRET_TOO_FEW_ARGUMENTS ,
  MAINRET_TOO_MANY_ARGUMENTS ,

  MAINRET_UNSUPPORT_EA ,
  MAINRET_UNSUPPORT_OPTION ,
  MAINRET_NOT_MULTITYPE_EA ,
  MAINRET_FAIL_TO_WRITE_EA ,
};

int main(int argc,char **argv)
{
  if( argc < 2 ){
    fputs("  __ __ __   ___ __  \n"
	  " /_ /_//_  / / // /  EAs,I do! 1.02 \n"
	  "/_ / /__/,/_/_//_/   (c) 1999,2001 HAYAMA,Kaoru\n"
	  "\n"
	  "usage:\n"
	  " (0) easido ............................ this help\n"
	  " (1) easido FILENAME ................... list FILENAME's EA\n"
	  " (2) easido FILENAME EATYPE\n"
	  "     easido -p FILENAME EATYPE ......... output EA to stdout\n"
	  " (3) easido FILENAME EATYPE VALUE ......"
	  " set VALUE to FILENAME's EA as EATYPE.\n"
	  " (4) easido -r FILENAME EATYPE ......... remove EA\n"
	  " (5) easido -c FILENAME EATYPE\n"
	  "     easido FILENAME EATYPE < EAFILE\n"
	  "     COMMAND | easido FILENAME EATYPE .. set values of stdin to ea"
	  " as EATYPE.\n"
	  " (6) easido -a FILENAME EATYPE VALUE ..."
	  " append VALUE as EATYPE of FILENAME\n"
	  "\n"
	  " Now supported EATYPEs are :\n" 
	  , stdout );
    for(EAManip **p=eaList ; *p != NULL ; ++p ){
      printf("\t%s\n", (*p)->getName() );
    }
    return MAINRET_NO_ARGUMENTS ;
  }
  try{
    if( argv[1][0] == '-' ){
      switch( tolower(argv[1][1]) ){
      case 'c':
	if( argc >= 4 ){
	  message_ignored( argv[0] , argc-4 , &argv[4] );
	  check_file_existance( argv[2] );
	  which_eatype(argv[3])->read(argv[2],stdin);
	}else{
	  fprintf(stderr,"%s -c FILENAME EATYPE\n",argv[0]);
	  return MAINRET_TOO_FEW_ARGUMENTS ;
	}
	break;
      case 'r':
	if( argc >= 4 ){
	  check_file_existance( argv[2] );
	  remove_ea(argv[2],argv[3]);
	  message_ignored( argv[0] , argc-4 , &argv[4] );
	}else{
	  fprintf(stderr,"%s -r FILENAME EATYPE\n",argv[0]);
	  return MAINRET_TOO_FEW_ARGUMENTS ;
	}
	break;
      case 'p':
	if( argc >= 4 ){
	  check_file_existance( argv[2]);
	  which_eatype(argv[3])->print(argv[2],stdout);
	  message_ignored( argv[0] , argc-4 , &argv[4] );
	}else{
	  fprintf(stderr,"%s -p FILENAME EATYPE\n",argv[0]);
	  return MAINRET_TOO_FEW_ARGUMENTS ;
	}
	break;
      case 'a':
	if( argc >= 4 ){
	  check_file_existance( argv[2] );
	  append_ea( argv[2] , argv[3] , argv[4] );
	  message_ignored( argv[0] , argc-5 , &argv[5] );
	}else{
	  fprintf(stderr,"%s -a FILENAME EATYPE APPENDED-TEXT\n",argv[0]);
	  return MAINRET_TOO_FEW_ARGUMENTS ;
	}
	break;

      default:
	fprintf(stderr,"%s: %c: Unknown option.\n"
		, argv[0] , argv[1][1] );
	return MAINRET_UNSUPPORT_OPTION ;
      }
    }else{
      switch( argc ){
      case 2:
	check_file_existance( argv[1] );
	listing( argv[1] );
	break;
      case 3:
	check_file_existance( argv[1] );
	if( isatty( fileno(stdin) ) ){
	  which_eatype(argv[2])->print(argv[1],stdout);
	}else{
	  which_eatype(argv[2])->read(argv[1],stdin);
	}
	break;
      case 4:
	check_file_existance( argv[1] );
	set_ea( argv[1] , argv[2] , argv[3] );
	break;
      default:
	fprintf(stderr,"%s: Too Many Argument.\n" , argv[0] );
	return MAINRET_TOO_MANY_ARGUMENTS ;
      }
    }
  }catch( EAManip::FailToWrite &e ){
    fprintf(  stderr,"%s: %s: fail to write EA.\n"
	    , argv[0] , e.fname );
    return MAINRET_FAIL_TO_WRITE_EA ;
  }catch( UnknownTypeEA &e ){
    fprintf(  stderr,"%s: %s: Unsupported type EA.\n"
	    , argv[0] , e.typeName );
    return MAINRET_UNSUPPORT_EA ;
  }catch( NotMultiTypeEA &e ){
    fprintf(stderr,"%s: %s: Can't append text to single type EA.\n"
	    , argv[0] , e.typeName );
    return MAINRET_NOT_MULTITYPE_EA ;
  }catch( NoSuchFileOrDirectory &e ){
    fprintf(stderr,"%s: %s: no such file or directory.\n"
	    , argv[0] , e.name() );
    return MAINRET_NO_SUCH_FILE_OR_DIRECTORY ;
  }
  return 0;
}
  

#include <sys/ea.h>
#include <stdlib.h>
#include <string.h>

#include "ealib.h"

/* ASCII タイプのＥＡ(.LONGNAME / .SUBJECT)を取得する。
 *	fname … ファイル名
 *	eatype … EAの属性名(".LONGNAME" や ".SUBJECT")
 * return
 *	得られた EA (char *)
 */
char *get_ascii_ea(  const char *fname , const char *eatype )
{
  struct _ea ea;
  union MultiPtr ptr;
  
  if( _ea_get( &ea , fname , 0 , eatype ) != 0 
     || ea.size <= 0 || ea.value == NULL )
    return NULL;

  ptr.value = ea.value;
  int type = *ptr.word++;
  if( type != EA_ASCIITYPE ){
    _ea_free(&ea);
    return NULL;
  }
  int size=*ptr.word++;
  char *eabuf=(char*)malloc( size + 1 );
  memcpy( eabuf , ptr.byte , size );
  eabuf[ size ] = '\0';
  
  _ea_free( &ea );
  return eabuf;
}

/* 拡張属性 .COMMENT の内容を得る。
 *	fname 属性を持つファイルの名前
 * return
 *	属性値をポインタ配列で返す。
 */
char **get_multiascii_ea( const char *fname  , const char *eatype )
{
  struct _ea ea;
  union MultiPtr ptr;
  
  if(   _ea_get( &ea , fname , 0 , eatype ) != 0
     || ea.size <= 0 || ea.value == NULL )
    return NULL;
  
  ptr.value = ea.value;
  if( *ptr.word++ != EA_MULTITYPE ){
    _ea_free( &ea );
    return NULL;
  }

  ptr.word++; /* コードページを読みとばす */
  int n=*ptr.word++; /* テーブルサイズ */
  char **table=(char**)malloc(sizeof(char*)*(n+1));
  if( table == NULL )
    return NULL;
  
  for(int i=0;i<n;i++){
    if( *ptr.word++ != EA_ASCIITYPE ){
      _ea_free( &ea );
      return NULL;
    }
    int size=*ptr.word++;
    if( (table[i] = (char*)malloc(size+1) ) != NULL ){
      memcpy( table[i] , ptr.byte , size );
      table[i][ size ] = '\0';
    }
    ptr.byte += size;
  }
  table[n] = NULL;
  _ea_free( &ea );
  return table;
}

void free_pointors(char **p)
{
  if( p != NULL ){
    for(char **q=p ; *q != NULL ; q++ )
      free(*q);
    free(p);
  }
}



/* ASCII タイプの拡張属性を設定する。
 *	fname	拡張属性を設定するファイルのファイル名
 *	eatype	拡張属性のタイプ(".SUBJECT"等)
 *	value	設定する内容。NULL の場合、その属性を削除する。
 */
int set_ascii_ea(  const char *fname 
		 , const char *eatype 
		 , const char *value )
{
  struct _ea eavalue;

  eavalue.flags = 0;
  if( value == 0 || value[0] == '\0' ){
    /* EA の値を消す */
    eavalue.size = 0;
    eavalue.value = "";
  }else{
    MultiPtr ptr;

    int len = strlen(value);
    ptr.value = eavalue.value = alloca( (eavalue.size = len+4)+1 );
    /* 4 はヘッダのバイト数 */
    
    *ptr.word++ = 0xFFFD;
    *ptr.word++ = len;
    memcpy(ptr.value , value , len );
  }
  return _ea_put( &eavalue , fname , 0 , eatype );
}


int set_multiascii_ea(  const char *fname 
		      , const char *eatype
		      , const char **values )
{
  int size=6; /* == 属性マーク＋コードページ＋ＥＡ数  */
  int n=0;
  for(const char **p=values; *p != NULL ; p++ ){
    size += strlen(*p) + 4; /* 4 = 属性マーク＋コードページ */
    ++n;
  }
  if( n==0 )
    return 0;
  
  struct _ea eavalue;
  MultiPtr ptr;
  
  eavalue.flags = 0;
  ptr.value = eavalue.value = alloca( (eavalue.size = size ) + 1 );
  
  *ptr.word++ = EA_MULTITYPE;
  *ptr.word++ = 0; /* code page */
  *ptr.word++ = n; /* EA の数 */
  for(const char **p=values ; *p != NULL ; p++ ){
    *ptr.word++ = EA_ASCIITYPE;
    unsigned short *toSize=ptr.word++;
    *toSize = 0;
    for(const char *sp=*p ; *sp != '\0' ; ++sp ){
      *ptr.byte++ = *sp;
      ++*toSize;
    }
  }
  return _ea_put(&eavalue,fname,0,eatype);
}

int unset_ea( const char *fname , const char *eatype )
{
  struct _ea eavalue;
  eavalue.flags = 0;
  eavalue.size  = 0;
  eavalue.value = "\0";
  return _ea_put( &eavalue , fname , 0 , eatype );
}

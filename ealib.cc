#include <sys/ea.h>
#include <stdlib.h>
#include <string.h>

#include "ealib.h"

/* ASCII �^�C�v�̂d�`(.LONGNAME / .SUBJECT)���擾����B
 *	fname �c �t�@�C����
 *	eatype �c EA�̑�����(".LONGNAME" �� ".SUBJECT")
 * return
 *	����ꂽ EA (char *)
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

/* �g������ .COMMENT �̓��e�𓾂�B
 *	fname ���������t�@�C���̖��O
 * return
 *	�����l���|�C���^�z��ŕԂ��B
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

  ptr.word++; /* �R�[�h�y�[�W��ǂ݂Ƃ΂� */
  int n=*ptr.word++; /* �e�[�u���T�C�Y */
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



/* ASCII �^�C�v�̊g��������ݒ肷��B
 *	fname	�g��������ݒ肷��t�@�C���̃t�@�C����
 *	eatype	�g�������̃^�C�v(".SUBJECT"��)
 *	value	�ݒ肷����e�BNULL �̏ꍇ�A���̑������폜����B
 */
int set_ascii_ea(  const char *fname 
		 , const char *eatype 
		 , const char *value )
{
  struct _ea eavalue;

  eavalue.flags = 0;
  if( value == 0 || value[0] == '\0' ){
    /* EA �̒l������ */
    eavalue.size = 0;
    eavalue.value = "";
  }else{
    MultiPtr ptr;

    int len = strlen(value);
    ptr.value = eavalue.value = alloca( (eavalue.size = len+4)+1 );
    /* 4 �̓w�b�_�̃o�C�g�� */
    
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
  int size=6; /* == �����}�[�N�{�R�[�h�y�[�W�{�d�`��  */
  int n=0;
  for(const char **p=values; *p != NULL ; p++ ){
    size += strlen(*p) + 4; /* 4 = �����}�[�N�{�R�[�h�y�[�W */
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
  *ptr.word++ = n; /* EA �̐� */
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

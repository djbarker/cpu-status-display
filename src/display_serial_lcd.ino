#include <LiquidCrystal.h>

/*
 * Parameters
 */
#define MAX_LEN 250 // maximum line length
unsigned long timeout = 60; // seconds
unsigned long refresh = 50; // milliseconds
unsigned long scroll = 400; // milliseconds

/*
 * Variables
 */
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

char line1[MAX_LEN];
char line2[MAX_LEN];
short pos1=0, pos2=0;
short len1=0, len2=0;
short active_line = 1;

unsigned long last_available = 0;
unsigned long last_refresh = 0;
unsigned long last_scroll = 0;
bool had_data = true;

/* Periodic memory copy function with padding.
 * 
 * dest - destination pointer
 * source - source pointer
 * pos - where to start copying from in source
 * num - number of data to copy from source to dest
 * len - the length of the source array
 * padding - what to pad with at the end
 * pad_length - how much to pad at the end
 */
void memcpyp(char* dest, char* source, unsigned int num, unsigned int pos, unsigned int len, char padding, unsigned char pad_len)
{
  if(pos+num<len)
  {
    memcpy(dest,source+pos,num);
  }
  else
  {
    int copied = 0;
    for(int i=0;i<num;++i)
    {
      if(pos+i<len)
      {
        dest[i] = source[pos+i];
      }
      else if(i<(len-pos)+pad_len)
      {
        dest[i] = padding;
      }
      else
      {
        dest[i] = source[copied];
        copied++;
      }
    }
  }
}

void setup()
{
  Serial.begin(9600); 
  lcd.begin(16, 2);
  lcd.clear();  
  for(int i=0;i<MAX_LEN;++i) line1[i] = line2[i] = ' ';
}

void loop()
{ 
  if(Serial.available()>0)
  {  
    last_available = millis();
    had_data = true;
    
    /* Read any available data. */
    while(Serial.available()>0)
    {  
      int ch = Serial.read();
      
      if(ch=='\r' || ch=='\t')
      {
        /* Ignore carriage returns and tabs. */
        continue;
      }
      else if(ch=='\n')
      {        
        delay(10); // allow serial buffer to catch up
        
        /* which line to update? */
        ch = Serial.read();
        
        switch(ch)
        {
        case (int)'T': // top line
          active_line = 1;
          memset(line1,' ',MAX_LEN);
          pos1 = len1 = 0;
          break;
        case (int)'B': // bottom line
          active_line = 2;
          memset(line2,' ',MAX_LEN);
          pos2 = len2 = 0;
          break;
        case (int)'D': // move top line down
          memcpy(line2,line1,MAX_LEN);
          pos2 = pos1;
          len2 = len1;
          active_line = 1;
          memset(line1,' ',MAX_LEN);
          pos1 = len1 = 0;
          break;
        default: // move bottom line up
          memcpy(line1,line2,MAX_LEN);
          pos1 = pos2;
          len1 = len2;
          active_line = 2;
          memset(line2,' ',MAX_LEN);
          pos2 = len2 = 0;
        }
      }
      else
      {
        /* Store new characters in the active line if there is space. */
        if(len1<MAX_LEN-1 && active_line==1)
        {
          line1[len1] = (char)ch;
          len1++;
        }
        else if(len2<MAX_LEN-1 && active_line==2)
        {
          line2[len2] = (char)ch;
          len2++;
        }
      }
    }
  }
  
  /* If enough time has elapsed and we have received some data refresh. */
  if((millis()-last_refresh)>=refresh && had_data)
  {
    char tmp[17];
    tmp[16] = '\0'; // zero terminate
    
    if(len1<=16) memcpy(tmp,line1,16);
    else         memcpyp(tmp,line1,16,pos1,len1,' ',4);
     
    lcd.setCursor(0,0);
    lcd.print(tmp);
    
    if(len2<=16) memcpy(tmp,line2,16);
    else         memcpyp(tmp,line2,16,pos2,len2,' ',4);
    
    lcd.setCursor(0,1);
    lcd.print(tmp);
    
    last_refresh = millis();
  }
  
  /* If enough time has elapsed scroll any long lines along by one character. */
  if((millis()-last_scroll)>=scroll && had_data)
  {
    if(len1>16) pos1 = (pos1+1)%(len1+4);
    if(len2>16) pos2 = (pos2+1)%(len2+4);
    
    last_scroll = millis();
  }

  /* If we haven't received any data for a while display the timeout message. */
  if((millis()-last_available)>=timeout*1000 && had_data)
  {
    lcd.setCursor(0,0);
    lcd.print("--- Awaiting ---");
    lcd.setCursor(0,1);
    lcd.print("--- data ... ---");
    had_data = false;
  }
}
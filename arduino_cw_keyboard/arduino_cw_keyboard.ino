/*
 * Based on code written by Mark VandeWtterning, k6hx, as found at:
 * http://brainwagon.org/2012/0131/an-arduino-powered-ibm-ps2-morse-keyboard/
 * 
 * Modified to put output to ic2d oled and (eventually) take usb keyboard
 * by ARS KE4MKG
 */



#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <avr/pgmspace.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C

 
#define QUEUESIZE       (128)
#define QUEUEMASK       (QUEUESIZE-1)

 
#define FREQ  (700)
#define WPM     (25)


#define DEFAULTWPM   15
#define MINWPM       5
#define MAXWPM       50


int wordsPerMinute = DEFAULTWPM;
int ditlen = 1200 / wordsPerMinute ;
boolean speedChange = false;

int pin = 4 ;                  // blink the LED for now... 
int tpin = 10 ;                 // tone pin
int aborted = 0 ;

int qhead = 0 ;
int qtail = 0 ;
char queue[QUEUESIZE] ;
int x, minX;


/* Create strings in progmem */
const char string_0[] PROGMEM = "CQ CQ CQ DE KE4MKG KE4MKG CQ CQ CQ DE KE4MKG KE4MKG K"; // "String 0" etc are strings to store - change to suit.
const char string_1[] PROGMEM = "QTH IS LAWRENCEVILLE, GA ? LAWRENCEVILLE, GA =";
const char string_2[] PROGMEM = "NAME IS JIM JIM =";
const char string_3[] PROGMEM = "RIG IS YAESU FT857 RUNNING 50 WATTS TO INV V AT 15 FEET=";
const char string_4[] PROGMEM = "MEMORY 5";
const char string_5[] PROGMEM = "MEMORY 6";


const char *const string_table[] PROGMEM = {string_0, string_1, string_2, string_3, string_4, string_5};

char buffer[75];

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT);


// LIBRARY OF CHARACTERS
// NOTE PROSIGNS ARE DEFINED IN send function
// below :

char ltab[] = {
    0b101,              // A
    0b11000,            // B 
    0b11010,            // C
    0b1100,             // D
    0b10,               // E
    0b10010,            // F
    0b1110,             // G
    0b10000,            // H
    0b100,              // I
    0b10111,            // J
    0b1101,             // K
    0b10100,            // L
    0b111,              // M
    0b110,              // N
    0b1111,             // O
    0b10110,            // P
    0b11101,            // Q
    0b1010,             // R
    0b1000,             // S
    0b11,               // T
    0b1001,             // U
    0b10001,            // V
    0b1011,             // W
    0b11001,            // X
    0b11011,            // Y
    0b11100             // Z
} ;
 
char ntab[] = {
    0b111111,           // 0
    0b101111,           // 1
    0b100111,           // 2
    0b100011,           // 3
    0b100001,           // 4
    0b100000,           // 5
    0b110000,           // 6
    0b111000,           // 7
    0b111100,           // 8
    0b111110            // 9
} ;


// FUNCTIONS ---------------------------------------------------------------------------------
void
lspace()
{
    mydelay(2*ditlen) ;
}
 
void
space()
{
    mydelay(4*ditlen) ;
}





// sendcode dispatches dits and dahs based binary sequence
// 0b101 drops the 0b1 and send dit and dah to pin.
void
sendcode(char code)
{
    
    int i ;
 
    for (i=7; i>= 0; i--)
        if (code & (1 << i))
            break ;
 
    for (i--; i>= 0; i--) {
        if (code & (1 << i))
            dah() ;
        else
           
            dit() ;
    }
    lspace() ;
}


// rd clears display of OLED
void
rd()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.display();
  x = 0;
}


 // send prints and counts the characters and if length greater than 39 it clears the OLED
 // it also sends the code via the sendcode function , also recognizes prosigns 
void
send(char ch)
{

// check for speed change:
  if(speedChange) {
    ditlen = 1200 / wordsPerMinute;
    speedChange = false;
  }

// check length of oled print and if g.t. 39 clear display else print
    if (x > 39) rd();
    display.print(ch);
    x = x + 1;

// send code    
    if (isalpha(ch)) {
        if (islower(ch)) ch = toupper(ch) ;
        sendcode(ltab[ch-'A']) ;
    } else if (isdigit(ch))
        sendcode(ntab[ch-'0']) ;
    else if (ch == ' ' || ch == '\r' || ch == '\n')
        space() ;
    else if (ch == '.')
        sendcode(0b1010101) ;
    else if (ch == ',')
        sendcode(0b1110011) ;
    else if (ch == '!')
        sendcode(0b1101011) ;
    else if (ch == '?')
        sendcode(0b1001100) ;
    else if (ch == '/')
        sendcode(0b110010) ;
    else if (ch == '+')      // AR
        sendcode(0b101010) ;
    else if (ch == '<')      // AR as well
        sendcode(0b101010) ;
    else if (ch == '>')      // SK
        sendcode(0b1000101) ; //
    else if (ch == '(')      // KN
        sendcode(0b110110) ; //       
    else if (ch == '-')
        sendcode(0b1100001) ;
    else if (ch == '=')
        sendcode(0b110001) ;
    else if (ch == '@')         // hardly anyone knows this!
        sendcode(0b1011010) ;
    else
        return ;                // ignore anything else
 
    if (!aborted) {
      Serial.print(ch) ;
      if (ch == 13) Serial.print((char) 10) ;
    }
    aborted = 0 ;
}






// ################################################
// MAIN                                           #
//#################################################


//SETUP --------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("KE4MKG");

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 17);
  display.println("KEYER");
  display.setTextSize(1.7);
  display.println("Lawrenceville");
  display.println("GA, USA");

  display.display();

  
  delay(5000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);


   x = 0;


}


void
queueadd(char ch)
{
    //Serial.print(ch);
    queue[qtail++] = ch ;
    //Serial.println(queue);
    qtail &= QUEUEMASK ;
}
 
void
queueadd(char *s)
{
  while (*s)
      queueadd(*s++) ;
}
 
char
queuepop()
{
    char ch ;
    ch = queue[qhead++] ;
    qhead &= QUEUEMASK ;

    return ch ;
}
 
int
queuefull()
{
    return (((qtail+1)%QUEUEMASK) == qhead) ;
}
 
int
queueempty()
{
    return (qhead == qtail) ;
}
 
void
queueflush()
{
    qhead = qtail ;
    rd();
    display.println("\nFLUSH!\n");
    display.display();
    delay(1000);
    rd();
}


void
mydelay(unsigned long ms)
{
    unsigned long t = millis() ;
    while (millis()-t < ms)
        ps2poll() ;
}

 
 
void
dit()
{
    
    digitalWrite(pin, HIGH) ;
    tone(tpin, FREQ) ;
    mydelay(ditlen) ;
    digitalWrite(pin, LOW) ;
    noTone(tpin) ;
    mydelay(ditlen) ;
 
}
 
void
dah()
{
   
    digitalWrite(pin, HIGH) ;
    tone(tpin, FREQ) ;
    mydelay(3*ditlen) ;
    digitalWrite(pin, LOW) ;
    noTone(tpin) ;
    mydelay(ditlen) ;
}


 
// ps2poll checks the input and will stop adding if que is full or 
//  abort loop or 
// send built in memory calls.
inline void
ps2poll()
{
    char ch ;
    int special = 0;

//    char M[] = {"CQ CQ CQ DE KE4MKG K\r\n"};
//    char N[] = {"QTH IS LAWRENCEVILLE, GA ? LAWRENCENVEVILLE, GA K\r\n"};
 
    while (Serial.available()) {
      
      if (special == 1){
        
        switch (ch=Serial.read()) {
        case '\061':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[0])));  
          queueadd(buffer) ;
          break; 
  
      
        case '\062':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[1])));  
          queueadd(buffer) ;
          break;

        case '\063':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[2])));  
          queueadd(buffer) ;
          break;

        
        case '\064':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[3])));  
          queueadd(buffer) ;
          break;  

        case '\065':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[4])));  
          queueadd(buffer) ;
          break;  

        case '\066':
          strcpy_P(buffer, (char *)pgm_read_word(&(string_table[5])));
          queueadd(buffer) ;
          break;  

        default:
          special = 0; 
          break; 
        
          }
             special = 0;
  
     
        }else{
   /* add break to normal path */
      
      
        
        if (queuefull()) {
            // Serial.println("QUEUE FULL");
            Serial.print("") ;
        } else {
        
            switch (ch=Serial.read()) {
            case '\033':
                queueflush() ;
                Serial.flush() ;
                Serial.println(F("== FLUSH ==")) ;
                aborted = 1 ;
                break ;

   
            case '\043':
                ChangeSendingSpeed();
                speedChange = true;
                break;
      
                
            case '\134': // if slash is pressed activate special code flag
              special = 1;
              Serial.println(F("== MACRO ==")) ;
              break;

            
            break ;
            default:
                queueadd(ch) ;
               
                break ;
            }
        }
    
    }
  }
}




void ChangeSendingSpeed(){
  char ch;
  int wereDone = 0;
  Serial.print(F(" words per minute at = "));
  Serial.println(wordsPerMinute);

  while(true){
    if(Serial.available()){
      ch = Serial.read();
      Serial.print(F("Speed change ch = "));
      Serial.println(wordsPerMinute);

      switch(ch) {
        case '>':
        if(wordsPerMinute < MAXWPM)
                  wordsPerMinute++;
                  break;
        case '<':
         if(wordsPerMinute > MINWPM)
                  wordsPerMinute--;
                  break;
        case '#':
          wereDone = 1;
          break;
       default:
          break;       
      }
      if (wereDone)
      break;

    }
  }
  ditlen = 1200/wordsPerMinute;
}

// LOOP -------------------------------------------------------------------------------------


void loop() {

   ps2poll() ;
      
    if (!queueempty())
        send(queuepop()) ;
 
  display.display();
  delay(20);
}

// #################################################

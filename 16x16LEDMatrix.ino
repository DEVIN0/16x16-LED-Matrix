// Written by Devin Hales
// Based on Michael Colton's DefconBadge code
// 2-bit gray-scale
// changed interrupt prescaler to 8 instead of 256
// constant interrupt intervals
// July 20, 2013


#define BUZZER            A4    //PF1
#define COL_DATA          10    //PB6
#define COL_OUTPUT_ENABLE A1    //PF6
#define COL_STROBE        A0    //PF7 Strobe basically STCP 
#define COL_CLOCK         13    //PC7 Clock input SHCP
#define COL_MEM_RESET     5     //PC6
#define ROW_DATA          9     //PB5
#define ROW_OUTPUT_ENABLE 8     //PB4
#define ROW_STROBE        6     //PD7
#define ROW_CLOCK         12    //PD6
#define ROW_MEM_RESET     4     //PD4
#define JOY_X             A2    //PF5
#define JOY_Y             A3    //PF4 maybe swapped with above, test it!
#define BATTERY_VOLTS     A5    //PF0
#define BUTTON_TOP        7     //PE6
//#define BUTTON_BOTTOM         //PD5

#define PORT0_SET         0b00000001
#define PORT1_SET         0b00000010
#define PORT2_SET         0b00000100
#define PORT3_SET         0b00001000
#define PORT4_SET         0b00010000
#define PORT5_SET         0b00100000
#define PORT6_SET         0b01000000
#define PORT7_SET         0b10000000

#define PORT0_MASK        0b11111110
#define PORT1_MASK        0b11111101
#define PORT2_MASK        0b11111011
#define PORT3_MASK        0b11110111
#define PORT4_MASK        0b11101111
#define PORT5_MASK        0b11011111
#define PORT6_MASK        0b10111111
#define PORT7_MASK        0b01111111

#define SHADES            2     // number of shades between off and full brightness

static char currentCol = 0;
static char dimSum = 0;
static char shadeCount = 0;
int x,y;

// write values between 0-7, 0: off, 1: dim, 7: bright
char frameBuffer[16][16] ={{0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0},
                           {0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0}};
                         


void setup()
{
  pinMode(BUZZER, OUTPUT);
  pinMode(COL_DATA, OUTPUT);
  pinMode(COL_OUTPUT_ENABLE, OUTPUT);
  pinMode(COL_STROBE, OUTPUT);
  pinMode(COL_CLOCK, OUTPUT);
  pinMode(COL_MEM_RESET, OUTPUT);
  pinMode(ROW_DATA, OUTPUT);
  pinMode(ROW_OUTPUT_ENABLE, OUTPUT);
  pinMode(ROW_STROBE, OUTPUT);
  pinMode(ROW_CLOCK, OUTPUT);
  pinMode(ROW_MEM_RESET, OUTPUT);
  pinMode(BUTTON_TOP, INPUT);
  pinMode(JOY_X, INPUT);
  pinMode(JOY_Y, INPUT);

  digitalWrite(BUZZER, HIGH);
  digitalWrite(BUTTON_TOP, HIGH);
  digitalWrite(COL_OUTPUT_ENABLE, LOW);
  digitalWrite(ROW_OUTPUT_ENABLE, LOW);
  digitalWrite(COL_MEM_RESET, HIGH);
  digitalWrite(ROW_MEM_RESET, HIGH);
  
  digitalWrite(COL_DATA, LOW);
  digitalWrite(COL_CLOCK, LOW);
  digitalWrite(ROW_DATA, LOW);
  digitalWrite(ROW_DATA, LOW);
  
  DDRD = DDRD & PORT5_MASK;    // PORTD5 input
  PORTD = PORTD | PORT5_SET;   // PORTD5 pull-up
  
  setupTimer();
}

void setupTimer()
{
//http://letsmakerobots.com/node/28278  
  
// initialize timer5 
  noInterrupts();             // disable all interrupts
  TCCR1A = 0;                 //
  TCCR1B = 0;                 //
  TCNT1  = 0;                 //

  //OCR1A = 31250;            // compare match register 16MHz/256/2Hz //or 8Mhz/256/1hz or 8Mhz/256/1hz
  OCR1A = 256;                 // compare match register
  TCCR1B |= (1 << WGM12);     // CTC mode (Clear timer on compare match)
  TCCR1B |= (1 << CS11);      // 8 prescaler 
  TIMSK1 |= (1 << OCIE1A);    // enable timer compare interrupt
  interrupts();               // enable all interrupts
}

ISR(TIMER1_COMPA_vect)
{ 

    //TCNT1 = 0;      // testing
    
    if(dimSum > SHADES)
    {
      //OCR1A = 256;
      dimSum = 0;
      writeCol();
      writeRowBuffer();
      strobe();
      dimSum++;
    }
    
    writeRowBuffer();
    strobe();
    dimSum++;              // this is the shade counter that gets compared to the frame buffer pixel values
   //     if(dimSum == 1) OCR1A = 128;
   // else if(dimSum == 2) OCR1A = 512;
   // TCNT1 = 0;
}

void writeCol()
{
  
  if(currentCol > 0)
  {
    currentCol --;
    PORTB = (PORTB | PORT6_SET);
  }
  else
  {
    currentCol = 15;
    PORTB = (PORTB & PORT6_MASK);    
  }

  PORTC = PORTC | PORT7_SET;  // clock
  PORTC = PORTC & PORT7_MASK;
  
}

void writeRowBuffer( )
{
  for(char i = 15; i >=0; i--)
  {
    if(frameBuffer[i][currentCol] > dimSum) PORTB = (PORTB | PORT5_SET);   // on
    else PORTB = (PORTB & PORT5_MASK);                                 // off
    
    PORTD = PORTD | PORT6_SET;       // clock
    PORTD = PORTD & PORT6_MASK;    
  } 
}

void strobe()
{
  PORTD = PORTD | PORT7_SET; 
  PORTF = PORTF | PORT7_SET; 
  PORTD = PORTD & PORT7_MASK;
  PORTF = PORTF & PORT7_MASK;
 }

void invert()
{
  for(int i = 0; i < 16; i++)
  {
    for(int j = 0; j < 16; j++)
    {
      frameBuffer[i][j] = 7 - frameBuffer[i][j];  
    }
  }  
}

void allShade()
{
  shadeCount++;
  if(shadeCount > (SHADES+1)) shadeCount = 0;
  for(int i = 0; i < 16; i++)
  {
    for(int j = 0; j < 16; j++)
    {
      frameBuffer[i][j] = shadeCount;  
    }
  }  
}

void loop()
{
  
  if(!(PIND & PORT5_SET)) invert();
  while(!(PIND & PORT5_SET)) delay(10);
    
  if(!(PINE & PORT6_SET)) allShade();
  while(!(PINE & PORT6_SET)) delay(10);
  
  static long lastTime = millis();
  
  if(millis() > lastTime + 10)
  {
    lastTime = millis();
    //x = analogRead(JOY_X)/64;
    //y = analogRead(JOY_Y)/64;
  }


}

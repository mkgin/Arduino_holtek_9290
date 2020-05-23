// bit bang holtek HT9290A/(B?)
// 
// A tone dialer IC
// 
//
// TODO:
// - fix store delay,do it in one place only (write5)
// - determine "reliable" delays for reading and writing  
// - check store_delay_us and  digit_delay_us used in righ place
// - improve reading , more debug info, timing between pulses, sync
// - control reading and writing from serial port

#include <Arduino.h>

// Arduino or other µcontroller pins
#define HOLTEK_CLOCK A4
#define HOLTEK_DATA  A5
#define HOLTEK_TRIG  7
#define SCOPE_DEBUG1 4
#define SCOPE_DEBUG2 5

// commands and keys excluding 1-9 and A-D which are calculated easily
#define HT9290_ZERO  B01010
#define HT9290_STAR  B01101
#define HT9290_HASH  B01100
#define HT9290_P1    B01011
#define HT9290_P2    B01110
#define HT9290_READ  B11000
#define HT9290_WRITE B11010
#define HT9290_STORE B00000

//const int half_clock_delay_us = 100;
const int half_clock_delay_us = 100;
const int default_half_clock_delay_us = 100;
const int store_delay_us = 600; 
const int digit_delay_us = 600;
const int read_interval_us = 15;

// 0 just setup and errors
// 1 stuff that shouldn't interfere with timing
// 2 stuff that interferes with timing
int debug = 0;                                                                                                                                                                                                              ;


//char ht_number[23] = "8675309";
const char ht_number[23] = "ABCD123456780";
byte ht_read_array[23];
int ht_read_pulse_times[23*5*2]; // pulse time counters


void ht_pin_mode( bool set_read = false)
{
  if (debug > 1)
  {
    Serial.println("ht_pin_mode():");
  }
  // set pins for writing, assume write as default
  //pinMode(HOLTEK_CLOCK, set_read == true ? INPUT_PULLUP : OUTPUT);
  //pinMode(HOLTEK_DATA, set_read == true ? INPUT_PULLUP : OUTPUT);
  pinMode(HOLTEK_CLOCK, set_read == true ? INPUT : OUTPUT);
  pinMode(HOLTEK_DATA, set_read == true ? INPUT : OUTPUT);
  if ( ! set_read )
  { 
    if (debug > 0)
    {
      Serial.println("ht_pin_mode(): Write");
    }
    digitalWrite(HOLTEK_CLOCK, HIGH);
    digitalWrite(HOLTEK_DATA, HIGH);
  }
  else if(debug > 1)
  {
      Serial.println("ht_pin_mode(): Read");
  }
}

void ht_write_bit(bool writeb = HIGH,  int half_clock = default_half_clock_delay_us )
{ 
  if (debug > 1)
    {
      Serial.print("WB:");
      Serial.print( (byte) writeb );
    }
  digitalWrite(HOLTEK_DATA, writeb);
  digitalWrite(HOLTEK_CLOCK, LOW);
  delayMicroseconds(half_clock);
  digitalWrite(HOLTEK_CLOCK, HIGH);
  delayMicroseconds(half_clock);
}

void ht_write_5bits(byte writebyte = 0, bool no_store_delay =false, int lessbits = 0 , int half_clock = default_half_clock_delay_us)
{
  if ( debug > 1 )
  { Serial.println();
    Serial.println(writebyte, BIN);
  }
  for (int bitshift = 4 ; bitshift >= lessbits ; bitshift--)
  {
    ht_write_bit( (bool) bitRead( writebyte, bitshift ) ,half_clock );    
  }
  if (no_store_delay == false)
  {
    delayMicroseconds(store_delay_us);
  }
}

int wait_for_clock( bool level = HIGH )
{
  int count = 0;
  while(( count <= 32000) && (digitalRead(HOLTEK_CLOCK) != level ))
  {
    delayMicroseconds( read_interval_us );
    count++;
  }
  if ( debug  > 1  )
  {
    // this will prevent reading properly
    Serial.print("\nwait_for_clock(");
    Serial.print( level ? "HIGH" : "LOW");
    Serial.print("): ");
    Serial.println( count );
  }
  if (count >= 32000)
  {
    Serial.println("wait_for_clock(): Timeout" );
  }
  return count;
}

void ht_read()
{
 // int count=0;
  //int readindex = 0;
  int bytecount;
  int bitshift;
  bool timeout= false;
  byte read_byte = 0;
  if ( debug != 0 )
  { 
    Serial.println("ht_read()");
  }
  digitalWrite(SCOPE_DEBUG1, LOW);
  // send read signal
  ht_write_5bits(HT9290_READ , true);
  //ht_write_5bits(HT9290_READ , true, 1); // 1 write 4 only
  ht_pin_mode(true); //set for reading
  digitalWrite(SCOPE_DEBUG1, HIGH);
  // data should be high and clock should be high
  for ( bytecount = 0 ; bytecount < 20; bytecount++)
  {
    timeout= false;
    read_byte=0;

    for( bitshift = 4; bitshift >= 0 ; bitshift--)
    {
      if (wait_for_clock(LOW) >= 32000 ) 
      {
        timeout = true;
        break;
      }
      if (wait_for_clock(HIGH) >= 32000 ) 
      {
        timeout = true;
        break;
      }
      if  (digitalRead(HOLTEK_DATA) == HIGH ) 
      {
        read_byte = read_byte | 1 << bitshift ;
      }
      ht_read_array[bytecount]=read_byte;
    }
    if ( timeout == true )
    {
      Serial.print("ht_read(): reading timed out!");
     // Serial.print( read_byte );
     // Serial.print(" bitshift = ");
     // Serial.println( bitshift );
      break;
    }
    //Serial.println("ht_read(): no break?");
  }
  Serial.print("ht_read(): reading timed out!\nread_byte = ");
  Serial.print( bytecount );
  Serial.print(" bitshift = ");
  Serial.println( bitshift );
  ht_pin_mode( false);
}

void ht_write_number()  //write number or command
{
  // 1-9 ... ASCII - 
  // 0,A,B C D # * from table
  // - short pause
  // p long pause
  int char_index = 0;
  if ( debug > 1 )
  {
    Serial.println("ht_write_number()");
  }
  // write write
  ht_write_5bits(HT9290_WRITE);
  for (int i = 0; i < 22; i++)
  {
    // get char read number array until 0, keep writing 0 until 22
    switch ( (char) ht_number[char_index] ) {
      case '0':
        ht_write_5bits( HT9290_ZERO );
        break;
      case '1' ... '9':
        // - 48
        ht_write_5bits( ht_number[char_index] - 48 );        
        break;
      case  '*':
        //
        ht_write_5bits( HT9290_STAR );
        break;
      case  '#':
        //
        ht_write_5bits( HT9290_HASH );
        break;
      case  'A' ...  'D':
        // - 64
        ht_write_5bits( ht_number[char_index] - 65 + 16 );
        break;
      case  '-': // short pause
        //
        ht_write_5bits( HT9290_P1 );
        break;
      case  'p': // long pause
        ht_write_5bits( HT9290_P2 );
        break;
      default: 
        ht_write_5bits( HT9290_STORE ); // zeroes
        break;
    }
    if ( ht_number[char_index] != 0 )
    {
      char_index++;
    }

  }
  delayMicroseconds(store_delay_us);
}

void ht_trigger()
{  if ( debug != 0 )
  {
    Serial.println("ht_trigger");
  }
    digitalWrite(HOLTEK_TRIG, LOW);
    delay(2000);
    digitalWrite(HOLTEK_TRIG, HIGH);
}

void setup() 
{
  Serial.begin(57600);
  Serial.println("\nHOLTEK HT9290 Test\n==================");
  // put your setup code here, to run once:
  ht_pin_mode( false); // handle clock and data in one place
  pinMode(HOLTEK_TRIG, OUTPUT );
  digitalWrite(HOLTEK_TRIG, HIGH);  
  pinMode(SCOPE_DEBUG1, OUTPUT );
  digitalWrite(SCOPE_DEBUG1, HIGH);   
    pinMode(SCOPE_DEBUG2, OUTPUT );
  digitalWrite(SCOPE_DEBUG2, HIGH);  
  if (debug != 0)
  {
    Serial.println("\nsetup():");
    delay(5000);
    Serial.println("setup():done delay");
  }
  ht_write_number();
    for ( int i = 0; i < 20 ; i++)
  {
    Serial.print(i);
    Serial.print(" - ");
    Serial.print(ht_number[i]);
    Serial.print(" - ");
    Serial.println( ht_number[i], BIN); 
  }
  delay(1000);
}

void ht_trigger_low(int milliseconds = 1000)
{
    {
    Serial.println("\nht_trigger_low():");
  }
    digitalWrite(HOLTEK_TRIG, LOW);
    delay(milliseconds);
    digitalWrite(HOLTEK_TRIG, HIGH);
    
}

void loop() {
  //delay(2000);
  //ht_write_number();
  // put your main code here, to run repeatedly:
  
  //ht_trigger_low(1000);
  delay(250);
  Serial.println("*");
  ht_read();
  for ( int i = 0; i < 20 ; i++)
  {
    Serial.print(i);
    Serial.print(" - ");
    Serial.print(ht_read_array[i]);
    Serial.print(" - ");
    Serial.println( ht_read_array[i], BIN); 
  }
  delay(6000);
}

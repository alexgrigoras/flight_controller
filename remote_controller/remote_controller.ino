/*
 * Arduino Transmitter
 * Using NRF2401
 * 
 * Author: Alexandru Grigoras
 * 
 * Libraries used: SPI, RF24(TMRh20)
 */

#include <SPI.h>  
#include <RF24.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(10, 9, 5, 4, 3, 2);

// Pins
RF24 myRadio (7, 8);                  // radio
const uint8_t pot_throttle = 0;       // potentiometer - throttle
const uint8_t pot_ailerons = 3;       // potentiometer - ailerons
const uint8_t pot_elevator = 2;       // potentiometer - elevator
const uint8_t pot_rudder = 1;         // potentiometer - rudder
const uint8_t button1 = A4;           // button1 - stabilization
const uint8_t button2 = A5;           // button2 - 
const uint8_t pot_1 = A6;             // potentiometer - stabilization
const uint8_t pot_2 = A7;             // potentiometer - stabilization
const uint8_t buzzer = 6;             // buzzer

int lastButtonState1 = HIGH;          // the previous reading from the input pin
int lastButtonState2 = HIGH;          // the previous reading from the input pin

// Declarations for NRF
byte addresses[][6] = {"0"};

// Liquid Crystal Characters

byte lcdChar[][8] = { 
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F},  
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00},  
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00},  
  { 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},  
  { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00},
  { 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00},  
  { 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  
  { 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  
} ;

/*
 * Structure for package transmitted
 */
typedef struct _package
{
  uint16_t id = 596;
  uint16_t escdata = 0;
  uint16_t servo_ailerons = 0;
  uint16_t servo_elevator = 0;
  uint16_t servo_rudder = 0;
  boolean stabilization = 0;
  boolean lighting = 0;
  uint16_t pot_1 = 0;
  uint16_t pot_2 = 0;
  // char  text[100] = "Text to be transmitted!";
} Package;

Package data;                         // Structure variable

uint8_t i;

boolean timeout;       // Timeout? True or False

void beep(unsigned char delayms){
  analogWrite(buzzer, 20);            // Almost any value can be used except 0 and 255
                                      // experiment to get the best tone
  delay(delayms);                     // wait for a delayms ms
  analogWrite(buzzer, 0);             // 0 turns it off
  delay(delayms);                     // wait for a delayms ms   
}  

void setup()
{
  // Serial
  //Serial.begin(115200);
  //delay(1000);
  
  pinMode(button1, INPUT);
  pinMode(buzzer, OUTPUT);

  // Radio
  myRadio.begin();  
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate(RF24_250KBPS); 
  myRadio.openWritingPipe(addresses[0]);
  delay(1000);

  // Create LCD Characters
  lcd.createChar(1, lcdChar[0]);
  lcd.createChar(2, lcdChar[1]);
  lcd.createChar(3, lcdChar[2]);
  lcd.createChar(4, lcdChar[3]);
  lcd.createChar(5, lcdChar[4]);
  lcd.createChar(6, lcdChar[5]);
  lcd.createChar(7, lcdChar[6]);
  lcd.createChar(8, lcdChar[7]);
  
  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print(" RC TRANSMITTER ");
  lcd.setCursor(0, 1);
  lcd.print("(c)Alex Grigoras");

  beep(30);
  delay(200);
  for(i = 0; i<4; i++)
  {
    beep(40);
  }
  beep(50);
  delay(100);
  beep(70);
  delay(1000);
  
  lcd.clear();
}

void loop()
{  
  // read the state of the switch into a local variable:
  data.stabilization = digitalRead(button1);
  if(lastButtonState1 == data.stabilization)
  {
    beep(30);
    lastButtonState1 = !data.stabilization;
  }
  data.lighting = digitalRead(button2);
  if(lastButtonState2 == data.lighting)
  {
    beep(30);
    lastButtonState2 = !data.lighting;
  }
  
  // Read potentiometer data
  data.escdata = analogRead(pot_throttle);
  data.servo_ailerons = analogRead(pot_ailerons);
  data.servo_elevator = analogRead(pot_elevator);
  data.servo_rudder = analogRead(pot_rudder);
  data.pot_1 = analogRead(pot_1);
  data.pot_2 = analogRead(pot_2);


  // Converting data from an interval to another
  data.escdata = map(data.escdata, 0, 1023, 32, 165);
  data.servo_ailerons = map(data.servo_ailerons, 0, 1023, 55, 125);
  data.servo_elevator = map(data.servo_elevator, 0, 1023, 135, 45);
  data.servo_rudder = map(data.servo_rudder, 0, 1023, 65, 115);
  data.pot_1 = map(data.pot_1, 0, 1023, 0, 99);
  data.pot_2 = map(data.pot_2, 0, 1023, 99, 0);

  
  // Send data through Package
  if(myRadio.write(&data, sizeof(data)))
    timeout = false;
  else
    timeout = true;

  // Serial debug
  /*
  Serial.print("\nPackage:");
  Serial.print(data.id);
  Serial.print("\n");
  Serial.print("\nMotor:");
  Serial.println(data.escdata);
  Serial.print("Ailerons:");
  Serial.println(data.servo_ailerons);
  Serial.print("Elevator:");
  Serial.println(data.servo_elevator);
  Serial.print("Rudder:");
  Serial.println(data.servo_rudder);
  */

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  
  lcd.setCursor(0, 0);
  lcd.print("T:");  
  data.escdata = map(data.escdata, 32, 165 , 1, 8);  
  lcd.print(char(data.escdata));
  lcd.setCursor(0, 1);
  lcd.print("E:");
  data.servo_elevator = map(data.servo_elevator, 135, 45 , 1, 8);
  lcd.print(char(data.servo_elevator));
  lcd.setCursor(4, 0);
  lcd.print("A:");
  data.servo_ailerons = map(data.servo_ailerons, 55, 125 , 1, 8);
  lcd.print(char(data.servo_ailerons));
  lcd.setCursor(4, 1);
  lcd.print("R:");
  data.servo_rudder = map(data.servo_rudder, 55, 125 , 1, 8);
  lcd.print(char(data.servo_rudder));
  lcd.setCursor(9, 0);
  lcd.print(data.pot_1); lcd.print(" ");
  lcd.setCursor(9, 1);
  lcd.print(data.pot_2); lcd.print(" ");
  
  if(data.stabilization == 1)
  {
    lcd.setCursor(15, 0);
    lcd.print("S");
  }
  else
  {
    lcd.setCursor(15, 0);
    lcd.print(" ");
  }
  if(data.lighting == 1)
  {
    lcd.setCursor(15, 1);
    lcd.print("L");
  }
  else
  {
    lcd.setCursor(15, 1);
    lcd.print(" ");
  }

  if(timeout)
  {
    lcd.setCursor(12, 0);
    lcd.print("NC");
  }
  else
  {
    lcd.setCursor(12, 0);
    lcd.print("C ");
  }
}

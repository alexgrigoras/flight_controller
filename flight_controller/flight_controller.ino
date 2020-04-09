/*
 * Arduino Receiver + Controller
 * Using NRF2401
 * 
 * Author: Alexandru Grigoras
 * 
 * Libraries used: SPI, RF24(TMRh20), Servo, Wire
 */
 
#include <SPI.h>  
#include <RF24.h> 
#include <Servo.h>
#include <Wire.h>

// Accelerometer + Gyroscope
const int MPU=0x68;         // I2C address of the MPU-6050

int16_t AcX,AcY,AcZ;
//Tmp,GyX,GyY,GyZ;

float Roll, Pitch;
float fXg = 0, fYg = 0, fZg = 0;

const float alpha = 0.5;

// Pins
RF24 myRadio (7, 8);        // radio

// Declarations
byte addresses[][6] = {"0"}; 

Servo esc;                  // Esc - Brushless motor
Servo s_ail_l;              // Servo motor - ailerons left
Servo s_ail_r;              // Servo motor - ailerons right
Servo s_ele;                // Servo motor - elevator
Servo s_rud;                // Servo motor - rudder
 
/*
 * Structure for package transmitted
 */
typedef struct _package
{
  uint16_t id = 0;
  uint16_t escdata = 0;
  uint16_t servo_ailerons = 0;
  uint16_t servo_elevator = 0;
  uint16_t servo_rudder = 0;
  boolean stabilization = 0;
  boolean lighting = 0;
  uint16_t pot_1 = 0;
  uint16_t pot_2 = 0;
  // char  text[100] = "empty";
} Package;

Package data;               // Structure variable

unsigned long started_waiting_at;
boolean timeout;       // Timeout? True or False

void setup() 
{
  /*
   * Initializations
   */
   
  // Gyroscope / Accelerometer
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  delay(500);
  
  // Radio
  myRadio.begin(); 
  myRadio.setChannel(115); 
  myRadio.setPALevel(RF24_PA_MAX);
  myRadio.setDataRate(RF24_250KBPS); 
  myRadio.openReadingPipe(1, addresses[0]);
  myRadio.startListening();
  delay(500);
   
  // ESC + Servo motors
  esc.attach(9);
  s_ail_l.attach(3);
  s_ail_r.attach(4);
  s_ele.attach(5);
  s_rud.attach(6);
  delay(100);

  // Serial
  //Serial.begin(115200);
  //delay(1000);
}

void loop()  
{
  // Check radio availability
  if ( myRadio.available()) 
  {
    // If available
    while (myRadio.available())
    {
      // Read data from package
      myRadio.read( &data, sizeof(data) );
    }
  }

  started_waiting_at = micros();               // timeout period, get the current microseconds
  timeout = false;                            //  variable to indicate if a response was received or not

  while ( ! myRadio.available() ) {                            // While nothing is received
    if (micros() - started_waiting_at > 200000 ) {           // If waited longer than 200ms, indicate timeout and exit while loop
      timeout = true;
      break;
    }
  }
  
  // Initialization communication to MPU6050
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);               // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU,6,true);   // request a total of 14 registers
  
  // Read Accelerometer Values
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  
  //Low Pass Filter
  fXg = AcX * alpha + (fXg * (1.0 - alpha));
  fYg = AcY * alpha + (fYg * (1.0 - alpha));
  fZg = AcZ * alpha + (fZg * (1.0 - alpha));

  //Roll & Pitch Calculations
  Roll  = (atan2(-fYg, fZg)*180.0)/M_PI;
  Pitch = (atan2(fXg, sqrt(fYg*fYg + fZg*fZg))*180.0)/M_PI;

  // If NO Connection
  if(timeout)
  {
    esc.write(32);
    s_rud.write(90);
    s_ail_l.write(90);
    s_ail_r.write(90);
    s_ele.write(90);
    //Serial.println("No connection"); 
  }
  else
    if(data.id == 596)
    {
      // Send data to ESC + servo  
      esc.write(data.escdata);
      s_rud.write(data.servo_rudder);
      
      if(data.servo_ailerons - (Roll/3)*data.stabilization >= 125 || data.servo_ailerons>=124)
      {
        s_ail_l.write(125);
        s_ail_r.write(125);
      }
      else
      {
        if(data.servo_ailerons - (Roll/3)*data.stabilization <= 55 || data.servo_ailerons<=56)
        {
          s_ail_l.write(55);
          s_ail_r.write(55);
        }
        else
        {
          s_ail_l.write(data.servo_ailerons - (Roll/3)*data.stabilization);
          s_ail_r.write(data.servo_ailerons - (Roll/3)*data.stabilization);
        }
      }
      
      if(data.servo_elevator-(Pitch/2)*data.stabilization >= 135 || data.servo_elevator>=134)
      {
        s_ele.write(135);
      }
      else
      {
        if(data.servo_elevator-(Pitch/2)*data.stabilization <= 45 || data.servo_elevator<=46)
        {
          s_ele.write(45);
        }
        else
        {
          s_ele.write(data.servo_elevator-(Pitch/2)*data.stabilization);
        }
      }

      /*
      Serial.println(data.escdata);
      Serial.println(data.servo_ailerons);
      Serial.println(data.servo_rudder);
      Serial.println(data.servo_elevator); 
      */
    }
    
    // Serial debug
    /* Not used right now
    Serial.print("\nPackage:");
    Serial.print(data.id);
    Serial.print("\n");
    Serial.println(Pitch);
    Serial.println(Roll);
    */
    
    // delay(100);
}

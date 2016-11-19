#include  "Arduino.h"
#include  "Support.h"
#include  "Pins.h"


void Configure_Pins()
{
  // Configure the motor
  pinMode(X_Stepper_Dir, OUTPUT);
  pinMode(Y_Stepper_Step, OUTPUT);
  pinMode(Y_Stepper_Dir, OUTPUT);    
  pinMode(X_Stepper_Step, OUTPUT);
  pinMode(X_Stepper_nEn, OUTPUT);
  pinMode(Y_Stepper_nEn, OUTPUT);

  digitalWrite(X_Stepper_nEn, HIGH);                                                // Disable steppers
  digitalWrite(Y_Stepper_nEn, HIGH);                                                
    
  // Configure the switch lines for pullup:
  pinMode(X_Limit_Switch, INPUT_PULLUP);
  pinMode(Y_Limit_Switch, INPUT_PULLUP);

  pinMode(Mode_Button, INPUT_PULLUP);
  pinMode(Mode_Button_Light, OUTPUT);
  digitalWrite(Mode_Button_Light, HIGH);
  
/*  
  pinMode(Left_Select_Button, INPUT_PULLUP);
  pinMode(Right_Select_Button, INPUT_PULLUP); 
  pinMode(Mode_Button, INPUT_PULLUP);  
  
  // Configure the Magnetoresitive Sensor lines as input pull ups.
  pinMode(Mag_Sense_0, INPUT_PULLUP);
  pinMode(Mag_Sense_1, INPUT_PULLUP);
  pinMode(Mag_Sense_2, INPUT_PULLUP);
  pinMode(Mag_Sense_3, INPUT_PULLUP);
  pinMode(Mag_Sense_4, INPUT_PULLUP);
  pinMode(Mag_Sense_5, INPUT_PULLUP);
  pinMode(Mag_Sense_6, INPUT_PULLUP);
  pinMode(Mag_Sense_7, INPUT_PULLUP);

  // Configure Pins to Darlingtons for the ground lines as outputs:
  pinMode(Darlington_Pin_0, OUTPUT);
  pinMode(Darlington_Pin_1, OUTPUT);
  pinMode(Darlington_Pin_2, OUTPUT);
  pinMode(Darlington_Pin_3, OUTPUT);
  pinMode(Darlington_Pin_4, OUTPUT);
  pinMode(Darlington_Pin_5, OUTPUT);
  pinMode(Darlington_Pin_6, OUTPUT);
  pinMode(Darlington_Pin_7, OUTPUT);

  // Configure Pins to LED anodes:
  pinMode(R_Channel_0, OUTPUT);
  pinMode(R_Channel_1, OUTPUT);
  pinMode(R_Channel_2, OUTPUT);
  pinMode(R_Channel_3, OUTPUT);
  pinMode(R_Channel_4, OUTPUT);
  pinMode(R_Channel_5, OUTPUT);
  pinMode(R_Channel_6, OUTPUT);
  pinMode(R_Channel_7, OUTPUT);

  pinMode(G_Channel_0, OUTPUT);
  pinMode(G_Channel_1, OUTPUT);
  pinMode(G_Channel_2, OUTPUT);
  pinMode(G_Channel_3, OUTPUT);
  pinMode(G_Channel_4, OUTPUT);
  pinMode(G_Channel_5, OUTPUT);
  pinMode(G_Channel_6, OUTPUT);
  pinMode(G_Channel_7, OUTPUT);

  pinMode(B_Channel_0, OUTPUT);
  pinMode(B_Channel_1, OUTPUT);
  pinMode(B_Channel_2, OUTPUT);
  pinMode(B_Channel_3, OUTPUT);
  pinMode(B_Channel_4, OUTPUT);
  pinMode(B_Channel_5, OUTPUT);
  pinMode(B_Channel_6, OUTPUT);
  pinMode(B_Channel_7, OUTPUT);
*/    
}

